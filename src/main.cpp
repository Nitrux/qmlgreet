#include <QQuickStyle>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QSettings>
#include <QFile>
#include <QCommandLineParser>
#include <QTextStream>
#include <QDateTime>
#include <QtGlobal>
#include <syslog.h>
#include "backend/AuthWrapper.h"
#include "backend/SessionModel.h"
#include "backend/UserModel.h"
#include "backend/SystemPower.h"
#include "backend/LayerShell.h"
#include "backend/SystemBattery.h"

// Custom message handler to redirect Qt debug output to syslog and file
void syslogMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(context);  // Suppress unused parameter warning

    QByteArray localMsg = msg.toLocal8Bit();
    const char *typeStr = "";
    int priority;

    switch (type) {
    case QtDebugMsg:
        priority = LOG_INFO;  // Use LOG_INFO instead of LOG_DEBUG to ensure it's logged
        typeStr = "DEBUG";
        break;
    case QtInfoMsg:
        priority = LOG_INFO;
        typeStr = "INFO";
        break;
    case QtWarningMsg:
        priority = LOG_WARNING;
        typeStr = "WARNING";
        break;
    case QtCriticalMsg:
        priority = LOG_ERR;
        typeStr = "CRITICAL";
        break;
    case QtFatalMsg:
        priority = LOG_CRIT;
        typeStr = "FATAL";
        break;
    default:
        priority = LOG_INFO;
        typeStr = "UNKNOWN";
    }

    // Log to syslog with type prefix
    syslog(priority, "[%s] %s", typeStr, localMsg.constData());

    // Also write to a dedicated log file
    static QFile logFile("/tmp/qmlgreet.log");
    bool canWriteLogFile = logFile.isOpen();
    if (!canWriteLogFile) {
        canWriteLogFile = logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
    }
    if (canWriteLogFile) {
        QTextStream out(&logFile);
        out << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")
            << " [" << typeStr << "] " << msg << "\n";
        out.flush();
    }

    // For fatal messages, abort as usual
    if (type == QtFatalMsg) {
        abort();
    }
}

int main(int argc, char *argv[])
{
    // Open syslog connection
    openlog("qmlgreet", LOG_PID | LOG_CONS, LOG_USER);

    // Install custom message handler
    qInstallMessageHandler(syslogMessageHandler);

    qInfo() << "qmlgreet starting...";
    qInfo() << "GREETD_SOCK environment variable:" << qgetenv("GREETD_SOCK");
    qInfo() << "Running as user:" << qgetenv("USER");

    QGuiApplication app(argc, argv);
    QQuickStyle::setStyle(QStringLiteral("org.mauikit.style"));
    app.setApplicationName("qmlgreet");
    app.setApplicationVersion("1.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("QML-based greeter for greetd");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption configOption(QStringList() << "c" << "config", "Path to config", "config", "/etc/qmlgreet/qmlgreet.conf");
    parser.addOption(configOption);
    parser.process(app);

    // Register QML types
    qmlRegisterType<AuthWrapper>("QmlGreet", 1, 0, "AuthWrapper");
    qmlRegisterType<SessionModel>("QmlGreet", 1, 0, "SessionModel");
    qmlRegisterType<UserModel>("QmlGreet", 1, 0, "UserModel");
    qmlRegisterType<SystemPower>("QmlGreet", 1, 0, "SystemPower");
    qmlRegisterType<LayerShell>("QmlGreet", 1, 0, "LayerShell");
    qmlRegisterType<SystemBattery>("QmlGreet", 1, 0, "SystemBattery");

    // Default Configuration
    QString configPath = parser.value(configOption);
    QString backgroundImagePath;
    QString defaultSession = "";
    QString avatarImagePath = "";
    bool showAvatars = true;
    bool debugBattery = false;
    bool blurEnabled = true;
    bool overlayEnabled = true;
    double overlayOpacity = 0.76;
    QString iconMode = QStringLiteral("system");
    bool lowercaseDate = false;
    // Load Configuration
    if (QFile::exists(configPath)) {
        QSettings config(configPath, QSettings::IniFormat);

        config.beginGroup("Appearance");
        backgroundImagePath = config.value("BackgroundImage", "").toString();
        avatarImagePath = config.value("AvatarImage", avatarImagePath).toString();
        blurEnabled = config.value("BlurEnabled", blurEnabled).toBool();
        overlayEnabled = config.value("OverlayEnabled", overlayEnabled).toBool();
        overlayOpacity = qBound(0.0, config.value("OverlayOpacity", overlayOpacity).toDouble(), 1.0);
        iconMode = config.value("IconMode", iconMode).toString().trimmed().toLower() == QStringLiteral("nerd")
            ? QStringLiteral("nerd") : QStringLiteral("system");
        config.endGroup();

        config.beginGroup("Debug");
        debugBattery = config.value("debugBattery", debugBattery).toBool();
        config.endGroup();

        config.beginGroup("Clock");
        lowercaseDate = config.value("LowercaseDate", lowercaseDate).toBool();
        config.endGroup();

        config.beginGroup("Behavior");
        showAvatars = config.value("ShowAvatars", showAvatars).toBool();
        config.endGroup();


        // Read DefaultSession from root level (QSettings doesn't recognize [General] group)
        defaultSession = config.value("DefaultSession", "").toString();
    }

    // Set background image
    UserModel userModel(avatarImagePath, &app);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("ConfigBackgroundImage", backgroundImagePath);
    engine.rootContext()->setContextProperty("ConfigShowAvatars", showAvatars);
    engine.rootContext()->setContextProperty("ConfigDebugBattery", debugBattery);
    engine.rootContext()->setContextProperty("ConfigBlurEnabled", blurEnabled);
    engine.rootContext()->setContextProperty("ConfigOverlayEnabled", overlayEnabled);
    engine.rootContext()->setContextProperty("ConfigOverlayOpacity", overlayOpacity);
    engine.rootContext()->setContextProperty("IconMode", iconMode);
    engine.rootContext()->setContextProperty("ConfigLowercaseDate", lowercaseDate);
    engine.rootContext()->setContextProperty("userModel", &userModel);
    engine.rootContext()->setContextProperty("ConfigDefaultSession", defaultSession);

    const QUrl url(QStringLiteral("qrc:/resources/qml/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl) QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);

    engine.load(url);

    int result = app.exec();

    // Close syslog connection
    closelog();

    return result;
}
