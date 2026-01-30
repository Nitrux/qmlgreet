#include "AuthWrapper.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QProcessEnvironment>
#include <QDataStream>
#include <QDebug>
#include <QSysInfo>
#include <QTimer>
#include <QProcess>
#include <QCoreApplication>

AuthWrapper::AuthWrapper(QObject *parent)
    : QObject(parent)
    , m_socket(new QLocalSocket(this))
{
    connect(m_socket, &QLocalSocket::readyRead, this, &AuthWrapper::onReadyRead);
    connect(m_socket, &QLocalSocket::errorOccurred, this, &AuthWrapper::onSocketError);
}

void AuthWrapper::login(const QString &username)
{
    // Prevent multiple login requests (e.g. double clicks) from firing simultaneously.
    if (m_processing) {
        qWarning() << "AuthWrapper: Login request ignored - already processing.";
        return;
    }

    m_processing = true;
    emit processingChanged();
    m_error = "";
    emit errorChanged();

    QString socketPath = qgetenv("GREETD_SOCK");
    if (socketPath.isEmpty()) {
        qWarning() << "AuthWrapper: No GREETD_SOCK found. Switching to MOCK MODE.";
        m_isMock = true;
        runMockLogin(username);
        return;
    }

    if (m_socket->state() != QLocalSocket::ConnectedState) {
        m_socket->connectToServer(socketPath);
        if (!m_socket->waitForConnected(1000)) {
            m_error = "Could not connect to greetd socket.";
            emit errorChanged();
            m_processing = false;
            emit processingChanged();
            return;
        }
    }

    QJsonObject request;
    request["type"] = "create_session";
    request["username"] = username;
    sendCommand(request);
}

void AuthWrapper::respond(const QString &response)
{
    // Guard: Don't respond if already processing or no prompt is active
    if (m_processing) {
        qWarning() << "AuthWrapper: Cannot respond while already processing";
        return;
    }

    if (m_prompt.isEmpty()) {
        qWarning() << "AuthWrapper: Cannot respond - no active prompt (session may have expired or been cancelled)";
        m_error = "No active authentication prompt. Please try logging in again.";
        emit errorChanged();
        return;
    }

    m_processing = true;
    emit processingChanged();

    if (m_isMock) {
        runMockResponse(response);
        return;
    }

    QJsonObject json;
    json["type"] = "post_auth_message_response";
    json["response"] = response;
    sendCommand(json);
}

void AuthWrapper::cancel()
{
    if (m_isMock) {
        reset();
        return;
    }

    if (m_socket->state() == QLocalSocket::ConnectedState) {
        QJsonObject json;
        json["type"] = "cancel_session";
        sendCommand(json);
    }
    reset();
}

void AuthWrapper::startSession(const QString &cmd)
{
    m_processing = true;
    emit processingChanged();

    if (m_isMock) {
        qDebug() << "Mock: Requesting launch of:" << cmd;
        // Simulate a short delay before "launching"
        QTimer::singleShot(500, this, [this](){
            qDebug() << "Mock: Session launched! (App would quit now)";
            QCoreApplication::quit();
        });
        return;
    }

    QStringList args = QProcess::splitCommand(cmd);
    
    QJsonArray cmdArray;
    for (const QString &arg : args) {
        cmdArray.append(arg);
    }

    // Protocol: { "type": "start_session", "cmd": ["/path/to/prog", "arg1", ...] }
    QJsonObject json;
    json["type"] = "start_session";
    json["cmd"] = cmdArray;
    
    sendCommand(json);
}

void AuthWrapper::runMockLogin(const QString &username)
{
    // Simulate network delay of 500ms
    QTimer::singleShot(500, this, [this, username]() {
        qDebug() << "Mock: Asking for password for" << username;
        m_prompt = "Mock Password (type 'fail' to error):";
        m_isSecret = true;
        m_processing = false;
        
        emit promptChanged();
        emit processingChanged();
    });
}

void AuthWrapper::runMockResponse(const QString &response)
{
    // Simulate processing delay
    QTimer::singleShot(600, this, [this, response]() {
        if (response == "fail") {
             qDebug() << "Mock: Authentication failed";
             m_error = "Mock Error: Invalid credentials (you typed 'fail')";
             emit errorChanged();
             
             // Reset UI to allow retry
             m_processing = false;
             emit processingChanged();
        } else {
             qDebug() << "Mock: Authentication successful";
             m_processing = false;
             emit processingChanged();
             emit loginSucceeded();
        }
    });
}

void AuthWrapper::sendCommand(const QJsonObject &json)
{
    QJsonDocument doc(json);
    QByteArray data = doc.toJson(QJsonDocument::Compact);
    quint32 len = data.size();

    // Protocol: 4-byte native-endian length + JSON payload
    QByteArray packet;
    QDataStream stream(&packet, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::ByteOrder(QSysInfo::ByteOrder));
    stream << len;
    packet.append(data);
    m_socket->write(packet);
    m_socket->flush();
}

void AuthWrapper::onReadyRead()
{
    m_buffer.append(m_socket->readAll());

    while (true) {
        // We need at least 4 bytes to read the length
        if (m_expectedLength == 0) {
            if (m_buffer.size() < 4) return; 

            QDataStream stream(m_buffer.left(4));
            stream.setByteOrder(QDataStream::ByteOrder(QSysInfo::ByteOrder));
            stream >> m_expectedLength;
            m_buffer.remove(0, 4);
        }

        // Do we have the full payload yet?
        if (m_buffer.size() < m_expectedLength) return; 

        QByteArray payload = m_buffer.left(m_expectedLength);
        m_buffer.remove(0, m_expectedLength);
        m_expectedLength = 0; 

        QJsonDocument doc = QJsonDocument::fromJson(payload);
        if (doc.isObject()) {
            processMessage(doc.object());
        }
    }
}

void AuthWrapper::processMessage(const QJsonObject &json)
{
    QString type = json["type"].toString();

    if (type == "success") {
        m_processing = false;
        emit processingChanged();
        emit loginSucceeded();
    } 
    else if (type == "auth_message") {
        QString msgType = json["auth_message_type"].toString();
        m_prompt = json["auth_message"].toString();
        m_isSecret = (msgType == "secret");
        m_processing = false;
        emit promptChanged();
        emit processingChanged();
    }
    else if (type == "error") {
        m_error = json["description"].toString();
        m_processing = false;
        emit errorChanged();
        emit processingChanged();
    }
}

void AuthWrapper::onSocketError(QLocalSocket::LocalSocketError)
{
    if (m_isMock) return; // Ignore socket errors in mock mode

    m_error = "Socket Error: " + m_socket->errorString();
    emit errorChanged();
    m_processing = false;
    emit processingChanged();
    reset();
}

void AuthWrapper::reset()
{
    m_prompt = "";
    m_error = "";
    m_isSecret = false;
    m_processing = false;
    m_expectedLength = 0;
    m_buffer.clear();
    m_isMock = false;
    
    emit promptChanged();
    emit processingChanged();
}
