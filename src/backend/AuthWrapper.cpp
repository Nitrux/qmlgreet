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
#include <QDir>
#include <QSettings>
#include <QDateTime>

AuthWrapper::AuthWrapper(QObject *parent)
    : QObject(parent)
    , m_socket(new QLocalSocket(this))
{
    connect(m_socket, &QLocalSocket::readyRead, this, &AuthWrapper::onReadyRead);
    connect(m_socket, &QLocalSocket::errorOccurred, this, &AuthWrapper::onSocketError);
}

void AuthWrapper::login(const QString &username)
{
    m_processing = true;
    emit processingChanged();
    m_error = "";
    emit errorChanged();

    QString socketPath = qgetenv("GREETD_SOCK");
    qDebug() << "AuthWrapper::login - GREETD_SOCK:" << socketPath;

    if (socketPath.isEmpty()) {
        qWarning() << "AuthWrapper: No GREETD_SOCK environment variable found. Switching to MOCK MODE.";
        m_isMock = true;
        runMockLogin(username);
        return;
    }

    if (m_socket->state() == QLocalSocket::ConnectedState) {
        m_socket->disconnectFromServer();
        if (m_socket->state() != QLocalSocket::UnconnectedState)
            m_socket->waitForDisconnected(1000);
    }

    if (m_socket->state() != QLocalSocket::ConnectedState) {
        qDebug() << "AuthWrapper: Connecting to socket:" << socketPath;
        m_socket->connectToServer(socketPath);
        if (!m_socket->waitForConnected(3000)) { 
            m_error = "Could not connect to greetd socket: " + m_socket->errorString();
            qWarning() << "AuthWrapper: Connection failed:" << m_error;
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
    if (m_processing) return;

    if (m_prompt.isEmpty()) {
        m_error = "No active authentication prompt.";
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
        QTimer::singleShot(500, this, [this](){
            QCoreApplication::quit();
        });
        return;
    }

    // Wrap the command to redirect output to a temp file for debugging
    QString logPath = QString("/tmp/qmlgreet-session-%1.log")
                      .arg(QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss"));
    QString wrappedCmd = QString("%1 > %2 2>&1").arg(cmd, logPath);

    QJsonArray cmdArray;
    cmdArray.append("/bin/sh");
    cmdArray.append("-c");
    cmdArray.append(wrappedCmd);

    // Load variables from /etc/environment so the session has a PATH, etc.
    QJsonArray envArray;
    QStringList envList = prepareEnv();
    for(const QString &e : envList) {
        envArray.append(e);
    }

    QJsonObject json;
    json["type"] = "start_session";
    json["cmd"] = cmdArray;
    json["env"] = envArray; // Send the loaded environment
    
    sendCommand(json);
}

// Helper to load environment variables
QStringList AuthWrapper::prepareEnv() {
    QStringList env;
    
    // 1. Load from /etc/environment
    QSettings envSett("/etc/environment", QSettings::IniFormat);
    for (const QString &key : envSett.allKeys()) {
        env << QString("%1=%2").arg(key, envSett.value(key).toString());
    }

    // 2. Load from /etc/environment.d/
    QDir envDir("/etc/environment.d");
    QFileInfoList infoList = envDir.entryInfoList(QDir::Files | QDir::Readable, QDir::Name);
    for (const QFileInfo &info : infoList) {
        QSettings s(info.filePath(), QSettings::IniFormat);
        for (const QString &key : s.allKeys()) {
             env << QString("%1=%2").arg(key, s.value(key).toString());
        }
    }

    // 3. Set Wayland variables
    env << "XDG_SESSION_TYPE=wayland";
    
    // 4. Pass through existing XDG_DATA_DIRS if set
    if (qEnvironmentVariableIsSet("XDG_DATA_DIRS"))
        env << "XDG_DATA_DIRS=" + qgetenv("XDG_DATA_DIRS");

    return env;
}

void AuthWrapper::runMockLogin(const QString &username)
{
    QTimer::singleShot(500, this, [this, username]() {
        m_prompt = "Mock Password (type 'fail' to error):";
        m_isSecret = true;
        m_processing = false;
        
        emit promptChanged();
        emit processingChanged();
    });
}

void AuthWrapper::runMockResponse(const QString &response)
{
    QTimer::singleShot(600, this, [this, response]() {
        if (response == "fail") {
             m_error = "Mock Error: Invalid credentials";
             emit errorChanged();
             m_processing = false;
             emit processingChanged();
        } else {
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

    QByteArray packet;
    QDataStream stream(&packet, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::ByteOrder(QSysInfo::ByteOrder));
    stream << len;
    packet.append(data);
    
    if (m_socket->state() == QLocalSocket::ConnectedState) {
        m_socket->write(packet);
        m_socket->flush();
    }
}

void AuthWrapper::onReadyRead()
{
    m_buffer.append(m_socket->readAll());

    while (true) {
        if (m_expectedLength == 0) {
            if (m_buffer.size() < 4) return; 

            QDataStream stream(m_buffer.left(4));
            stream.setByteOrder(QDataStream::ByteOrder(QSysInfo::ByteOrder));
            stream >> m_expectedLength;
            m_buffer.remove(0, 4);
        }

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
        m_prompt = "";
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
        m_prompt = "";
        m_error = json["description"].toString();
        m_processing = false;
        emit errorChanged();
        emit processingChanged();
    }
}

void AuthWrapper::onSocketError(QLocalSocket::LocalSocketError)
{
    if (m_isMock) return;

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
    // Keep mock state if it was mock
    emit promptChanged();
    emit processingChanged();
}
