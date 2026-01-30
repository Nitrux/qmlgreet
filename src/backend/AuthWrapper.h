#pragma once

#include <QObject>
#include <QLocalSocket>
#include <QJsonObject>
#include <QByteArray>
#include <QStringList>

/**
 * @brief The bridge between QML and the greetd IPC socket.
 * Handles authentication (login/password) and launching the final session.
 */
class AuthWrapper : public QObject
{
    Q_OBJECT
    
    // UI Properties
    Q_PROPERTY(QString currentPrompt READ currentPrompt NOTIFY promptChanged)
    Q_PROPERTY(bool isSecret READ isSecret NOTIFY promptChanged)
    Q_PROPERTY(QString error READ error WRITE setError NOTIFY errorChanged)
    Q_PROPERTY(bool processing READ processing NOTIFY processingChanged)

public:
    explicit AuthWrapper(QObject *parent = nullptr);

    // Property Getters
    QString currentPrompt() const { return m_prompt; }
    bool isSecret() const { return m_isSecret; }
    QString error() const { return m_error; }
    bool processing() const { return m_processing; }

    // Property Setters
    void setError(const QString &error) {
        if (m_error != error) {
            m_error = error;
            emit errorChanged();
        }
    }

    /**
     * @brief Initiates the login process for a specific user.
     * @param username The user to log in (e.g., "root", "jdoe").
     */
    Q_INVOKABLE void login(const QString &username);

    /**
     * @brief Sends the password or answer to the daemon.
     * @param response The text typed by the user.
     */
    Q_INVOKABLE void respond(const QString &response);

    /**
     * @brief Cancels the current auth session and resets the UI.
     */
    Q_INVOKABLE void cancel();

    /**
     * @brief Launches the actual desktop environment.
     * Call this ONLY after loginSucceeded() is emitted.
     * @param cmd The command string (e.g. "/usr/bin/startplasma-wayland")
     */
    Q_INVOKABLE void startSession(const QString &cmd);

signals:
    void promptChanged();
    void errorChanged();
    void processingChanged();
    
    // Emitted when authentication is 100% complete.
    // The UI should listen for this and then call startSession().
    void loginSucceeded();

private slots:
    void onReadyRead();
    void onSocketError(QLocalSocket::LocalSocketError socketError);

private:
    // Helpers
    void sendCommand(const QJsonObject &json);
    void processMessage(const QJsonObject &json);
    void reset();
    
    QStringList prepareEnv();

    // Mock Helpers
    void runMockLogin(const QString &username);
    void runMockResponse(const QString &response);

    QLocalSocket *m_socket;

    // Internal State
    QString m_prompt;
    QString m_error;
    bool m_isSecret = false;
    bool m_processing = false;
    bool m_isMock = false;
    bool m_sessionStarting = false;

    // Buffer for incoming JSON packets
    QByteArray m_buffer;
    quint32 m_expectedLength = 0;
};
