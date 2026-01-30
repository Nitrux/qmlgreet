#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QSettings>
#include <QFile>
#include <QCommandLineParser>
#include <QPalette>
#include <QQuickStyle>
#include <QIcon>
#include <QFont>
#include <syslog.h>
#include "backend/AuthWrapper.h"
#include "backend/SessionModel.h"
#include "backend/UserModel.h"
#include "backend/SystemPower.h"
#include "backend/LayerShell.h"
#include "backend/ColorSchemeLoader.h"
#include "backend/SystemBattery.h"

// Custom message handler to redirect Qt debug output to syslog
void syslogMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    int priority;

    switch (type) {
    case QtDebugMsg:
        priority = LOG_DEBUG;
        break;
    case QtInfoMsg:
        priority = LOG_INFO;
        break;
    case QtWarningMsg:
        priority = LOG_WARNING;
        break;
    case QtCriticalMsg:
        priority = LOG_ERR;
        break;
    case QtFatalMsg:
        priority = LOG_CRIT;
        break;
    default:
        priority = LOG_INFO;
    }

    syslog(priority, "%s", localMsg.constData());

    // For fatal messages, abort as usual
    if (type == QtFatalMsg) {
        abort();
    }
}

int main(int argc, char *argv[])
{
    // Open syslog connection
    openlog("qmlgreet", LOG_PID | LOG_CONS, LOG_AUTH);

    // Install custom message handler
    qInstallMessageHandler(syslogMessageHandler);

    // Force Breeze style for Qt Quick Controls if not set via env
    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE")) {
        QQuickStyle::setStyle("org.kde.desktop");
    }

    QGuiApplication app(argc, argv);
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

    ColorSchemeLoader *colorScheme = new ColorSchemeLoader(&app);

    // Default Configuration
    QString configPath = parser.value(configOption);
    QString colorSchemePath = "/usr/share/color-schemes/QMLGreetDefault.colors";
    QString backgroundImagePath;
    QString iconTheme = "hicolor"; 
    QString fontName = "Noto Sans";
    int fontSize = 10;
    QString defaultSession = "";

    // Load Configuration
    if (QFile::exists(configPath)) {
        QSettings config(configPath, QSettings::IniFormat);

        config.beginGroup("Appearance");
        colorSchemePath = config.value("ColorScheme", colorSchemePath).toString();
        backgroundImagePath = config.value("BackgroundImage", "").toString();
        iconTheme = config.value("IconTheme", iconTheme).toString();
        fontName = config.value("Font", fontName).toString();
        fontSize = config.value("FontSize", fontSize).toInt();
        config.endGroup();

        // Read DefaultSession from root level (QSettings doesn't recognize [General] group)
        defaultSession = config.value("DefaultSession", "").toString();
    }

    // Set font
    if (!fontName.isEmpty()) {
        QFont font(fontName);
        if (fontSize > 0) font.setPointSize(fontSize);
        app.setFont(font);
    }

    // Set icon theme
    if (!iconTheme.isEmpty()) {
        QIcon::setThemeName(iconTheme);
    }

    // Set color scheme
    QPalette palette;
    if (QFile::exists(colorSchemePath)) {
        palette = colorScheme->loadColorScheme(colorSchemePath);
        app.setPalette(palette);
    }

    // Set background image
    if (!backgroundImagePath.isEmpty() && QFile::exists(backgroundImagePath)) {
        colorScheme->setBackgroundImage(backgroundImagePath);
    }

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("ColorScheme", colorScheme);
    
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
