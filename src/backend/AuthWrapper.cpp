// SPDX-License-Identifier: BSD-3-Clause
// Copyright 2026 Nitrux Latinoamericana S.C. <hello@nxos.org>

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
#include <QSettings>
#include <QDir>
#include <QFileInfo>

AuthWrapper::AuthWrapper(QObject *parent)
    : QObject(parent)
    , m_socket(new QLocalSocket(this))
{
    connect(m_socket, &QLocalSocket::readyRead, this, &AuthWrapper::onReadyRead);
    connect(m_socket, &QLocalSocket::errorOccurred, this, &AuthWrapper::onSocketError);
}

void AuthWrapper::login(const QString &username)
{
    qDebug() << "AuthWrapper: login() called with username:" << username;

    // Prevent multiple login requests (e.g. double clicks) from firing simultaneously.
    if (m_processing) {
        qWarning() << "AuthWrapper: Login request ignored - already processing.";
        return;
    }

    m_processing = true;
    Q_EMIT processingChanged();
    m_error = "";
    Q_EMIT errorChanged();

    QString socketPath = qgetenv("GREETD_SOCK");
    qDebug() << "AuthWrapper: GREETD_SOCK =" << socketPath;

    if (socketPath.isEmpty()) {
        qWarning() << "AuthWrapper: No GREETD_SOCK found. Switching to MOCK MODE.";
        m_isMock = true;
        runMockLogin(username);
        return;
    }

    if (m_socket->state() != QLocalSocket::ConnectedState) {
        qDebug() << "AuthWrapper: Connecting to greetd socket...";
        m_socket->connectToServer(socketPath);
        if (!m_socket->waitForConnected(1000)) {
            m_error = "Could not connect to greetd socket.";
            Q_EMIT errorChanged();
            m_processing = false;
            Q_EMIT processingChanged();
            return;
        }
        qDebug() << "AuthWrapper: Connected to greetd socket";
    }

    QJsonObject request;
    request["type"] = "create_session";
    request["username"] = username;
    qDebug() << "AuthWrapper: Sending create_session request for user:" << username;
    sendCommand(request);
}

void AuthWrapper::respond(const QString &response)
{
    qDebug() << "AuthWrapper: respond() called";

    // Guard: Don't respond if already processing or no prompt is active
    if (m_processing) {
        qWarning() << "AuthWrapper: Cannot respond while already processing";
        return;
    }

    if (m_prompt.isEmpty()) {
        qWarning() << "AuthWrapper: Cannot respond - no active prompt (session may have expired or been cancelled)";
        m_error = "No active authentication prompt. Please try logging in again.";
        Q_EMIT errorChanged();
        return;
    }

    m_processing = true;
    Q_EMIT processingChanged();

    if (m_isMock) {
        runMockResponse(response);
        return;
    }

    QJsonObject json;
    json["type"] = "post_auth_message_response";
    json["response"] = response;
    qDebug() << "AuthWrapper: Sending password response to greetd";
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
    m_sessionStarting = true;
    Q_EMIT processingChanged();

    // Safety check for empty commands
    if (cmd.trimmed().isEmpty()) {
        qWarning() << "AuthWrapper: startSession called with empty command!";
        m_error = "Internal Error: No session command provided.";
        Q_EMIT errorChanged();
        m_processing = false;
        m_sessionStarting = false;
        Q_EMIT processingChanged();
        return;
    }

    qDebug() << "AuthWrapper: Starting session with command:" << cmd;

    if (m_isMock) {
        qDebug() << "Mock: Requesting launch of:" << cmd;
        // Simulate a short delay before "launching"
        QTimer::singleShot(500, this, [this](){
            qDebug() << "Mock: Session launched! (App would quit now)";
            QCoreApplication::quit();
        });
        return;
    }

    // Split the command string into executable + args
    // e.g. "dbus-run-session sway" becomes ["dbus-run-session", "sway"]
    QStringList args = QProcess::splitCommand(cmd);
    QJsonArray cmdArray;
    for (const QString &arg : args) {
        cmdArray.append(arg);
    }

    qDebug() << "AuthWrapper: Command split into" << args.size() << "arguments:" << args;

    // Prepare environment variables
    QStringList envList = prepareEnv();
    QJsonArray envArray;
    for (const QString &envVar : envList) {
        envArray.append(envVar);
    }

    qDebug() << "AuthWrapper: Prepared" << envList.size() << "environment variables";

    // Protocol: { "type": "start_session", "cmd": ["prog", "arg1", ...], "env": ["VAR=value", ...] }
    QJsonObject json;
    json["type"] = "start_session";
    json["cmd"] = cmdArray;

    if (!envArray.isEmpty()) {
        json["env"] = envArray;
    }

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
        
        Q_EMIT promptChanged();
        Q_EMIT processingChanged();
    });
}

void AuthWrapper::runMockResponse(const QString &response)
{
    // Simulate processing delay
    QTimer::singleShot(600, this, [this, response]() {
        if (response == "fail") {
             qDebug() << "Mock: Authentication failed";
             m_error = "Mock Error: Invalid credentials (you typed 'fail')";
             Q_EMIT errorChanged();
             
             // Reset UI to allow retry
             m_processing = false;
             Q_EMIT processingChanged();
        } else {
             qDebug() << "Mock: Authentication successful";
             m_processing = false;
             Q_EMIT processingChanged();
             Q_EMIT loginSucceeded();
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
    qDebug() << "AuthWrapper: Received message from greetd:" << QJsonDocument(json).toJson(QJsonDocument::Compact);

    QString type = json["type"].toString();

    if (type == "success") {
        if (m_canceling) {
            // Successfully canceled the session after an error
            qDebug() << "AuthWrapper: Session canceled successfully, resetting for retry";
            m_canceling = false;
            m_processing = false;
            m_prompt = "";

            // Close and reset the socket to allow fresh login attempts
            if (m_socket->state() == QLocalSocket::ConnectedState) {
                m_socket->disconnectFromServer();
            }

            Q_EMIT processingChanged();
            Q_EMIT promptChanged();
            // Don't emit loginSucceeded - the error was already set
            return;
        }

        if (m_sessionStarting) {
            qDebug() << "AuthWrapper: Session started successfully, quitting greeter";
            // Session started successfully - greetd will now launch the session
            // The greeter should exit
            QCoreApplication::quit();
        } else {
            qDebug() << "AuthWrapper: Authentication successful, emitting loginSucceeded signal";
            m_processing = false;
            Q_EMIT processingChanged();
            Q_EMIT loginSucceeded();
        }
    }
    else if (type == "auth_message") {
        QString msgType = json["auth_message_type"].toString();
        m_prompt = json["auth_message"].toString();
        m_isSecret = (msgType == "secret");

        // Handle info and error message types
        if (msgType == "info") {
            qDebug() << "Auth info:" << m_prompt;
            // Still allow continuation for info messages
        }
        else if (msgType == "error") {
            qWarning() << "Auth error message:" << m_prompt;
            m_error = m_prompt;
            Q_EMIT errorChanged();
        }

        m_processing = false;
        Q_EMIT promptChanged();
        Q_EMIT processingChanged();
    }
    else if (type == "error") {
        QString errorType = json["error_type"].toString();
        QString description = json["description"].toString();

        // Provide user-friendly error messages
        if (errorType == "auth_error") {
            m_error = "Incorrect password. Please try again.";
        } else if (description.contains("Connection refused") || description.contains("os error 111")) {
            m_error = "Session error. Please try logging in again.";
        } else {
            m_error = description;
        }

        qWarning() << "greetd error:" << errorType << "-" << description;

        m_processing = false;
        m_sessionStarting = false;
        Q_EMIT errorChanged();
        Q_EMIT processingChanged();

        // Cancel the session on error and reset for retry
        if (m_socket->state() == QLocalSocket::ConnectedState) {
            m_canceling = true;
            QJsonObject cancelJson;
            cancelJson["type"] = "cancel_session";
            sendCommand(cancelJson);
        } else {
            // Socket already closed, just reset to allow retry
            m_canceling = false;
            m_prompt = "";
            Q_EMIT promptChanged();
        }
    }
}

void AuthWrapper::onSocketError(QLocalSocket::LocalSocketError)
{
    if (m_isMock) return; // Ignore socket errors in mock mode

    m_error = "Socket Error: " + m_socket->errorString();
    Q_EMIT errorChanged();
    m_processing = false;
    Q_EMIT processingChanged();
    reset();
}

void AuthWrapper::reset()
{
    m_prompt = "";
    m_error = "";
    m_isSecret = false;
    m_processing = false;
    m_sessionStarting = false;
    m_canceling = false;
    m_expectedLength = 0;
    m_buffer.clear();
    m_isMock = false;

    Q_EMIT promptChanged();
    Q_EMIT processingChanged();
}

QStringList AuthWrapper::prepareEnv()
{
    QStringList env;
    const QStringList sessionSpecificVariables = {
        "DBUS_SESSION_BUS_ADDRESS",
        "DISPLAY",
        "GREETD_SOCK",
        "HYPRLAND_INSTANCE_SIGNATURE",
        "PATH",
        "WAYLAND_DISPLAY",
        "XDG_RUNTIME_DIR",
        "XDG_SEAT",
        "XDG_SEAT_PATH",
        "XDG_SESSION_ID",
        "XDG_SESSION_PATH",
        "XDG_SESSION_TYPE",
        "XDG_VTNR",
    };

    // 1. Read from /etc/environment
    QSettings envSett("/etc/environment", QSettings::IniFormat);
    for (const QString &key : envSett.allKeys()) {
        if (!sessionSpecificVariables.contains(key))
            env << QString("%1=%2").arg(key).arg(envSett.value(key).toString());
    }

    // 2. Read from /etc/environment.d/*.conf files
    QDir envDir("/etc/environment.d");
    if (envDir.exists()) {
        QFileInfoList files = envDir.entryInfoList(QDir::Files | QDir::Readable, QDir::Name);
        for (const QFileInfo &info : files) {
            QSettings s(info.filePath(), QSettings::IniFormat);
            for (const QString &key : s.allKeys()) {
                if (!sessionSpecificVariables.contains(key))
                    env << QString("%1=%2").arg(key).arg(s.value(key).toString());
            }
        }
    }

    // 3. Inherit only non-session-specific values from the greeter.
    // The authenticated login must provide its own runtime, seat, and VT context.
    if (qEnvironmentVariableIsSet("PATH"))
        env << "PATH=" + QString::fromLocal8Bit(qgetenv("PATH"));

    // 4. Force Wayland session type (recommended for Nitrux/Maui)
    env << "XDG_SESSION_TYPE=wayland";

    return env;
}
