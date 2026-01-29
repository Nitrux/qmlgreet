#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QSettings>
#include <QFile>
#include <QCommandLineParser>
#include "backend/AuthWrapper.h"
#include "backend/SessionModel.h"
#include "backend/UserModel.h"
#include "backend/SystemPower.h"
#include "backend/LayerShell.h"
#include "backend/ColorSchemeLoader.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName("qmlgreet");
    app.setApplicationVersion("1.0");

    // Parse command-line arguments
    QCommandLineParser parser;
    parser.setApplicationDescription("QML-based greeter for greetd");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption configOption(QStringList() << "c" << "config",
        "Path to configuration file",
        "config",
        "/etc/qmlgreet/qmlgreet.conf");
    parser.addOption(configOption);

    parser.process(app);

    qmlRegisterType<AuthWrapper>("QmlGreet", 1, 0, "AuthWrapper");
    qmlRegisterType<SessionModel>("QmlGreet", 1, 0, "SessionModel");
    qmlRegisterType<UserModel>("QmlGreet", 1, 0, "UserModel");
    qmlRegisterType<SystemPower>("QmlGreet", 1, 0, "SystemPower");
    qmlRegisterType<LayerShell>("QmlGreet", 1, 0, "LayerShell");

    // Load color scheme from configuration
    ColorSchemeLoader *colorScheme = new ColorSchemeLoader(&app);

    // Load configuration file from command-line argument or default path
    QString configPath = parser.value(configOption);
    QString colorSchemePath = "/usr/share/color-schemes/CatppuccinMochaNitrux.colors";

    if (QFile::exists(configPath)) {
        QSettings config(configPath, QSettings::IniFormat);
        config.beginGroup("Appearance");
        colorSchemePath = config.value("ColorScheme", colorSchemePath).toString();
        config.endGroup();
    } else if (configPath != "/etc/qmlgreet/qmlgreet.conf") {
        // Warn only if user explicitly specified a config file that doesn't exist
        qWarning() << "Config file not found at:" << configPath;
    }

    // Load the color scheme
    if (QFile::exists(colorSchemePath)) {
        colorScheme->loadColorScheme(colorSchemePath);
    } else {
        qWarning() << "Color scheme not found at:" << colorSchemePath;
        qWarning() << "Using built-in defaults";
    }

    QQmlApplicationEngine engine;

    // Make color scheme available to QML
    engine.rootContext()->setContextProperty("ColorScheme", colorScheme);

    const QUrl url(QStringLiteral("qrc:/resources/qml/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);

    engine.load(url);

    return app.exec();
}
