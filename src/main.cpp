#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QSettings>
#include <QFile>
#include <QCommandLineParser>
#include <QPalette>
#include <QQuickStyle>
#include "backend/AuthWrapper.h"
#include "backend/SessionModel.h"
#include "backend/UserModel.h"
#include "backend/SystemPower.h"
#include "backend/LayerShell.h"
#include "backend/ColorSchemeLoader.h"
#include "backend/SystemBattery.h"

int main(int argc, char *argv[])
{
    // Set Qt Quick Controls style before creating QGuiApplication
    QQuickStyle::setStyle("org.kde.desktop");

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
    qmlRegisterType<SystemBattery>("QmlGreet", 1, 0, "SystemBattery");

    // Load color scheme from configuration
    ColorSchemeLoader *colorScheme = new ColorSchemeLoader(&app);

    // Load configuration file from command-line argument or default path
    QString configPath = parser.value(configOption);
    QString colorSchemePath = "/usr/share/color-schemes/CatppuccinMochaNitrux.colors";
    QString backgroundImagePath;

    qDebug() << "Reading config from:" << configPath;

    if (QFile::exists(configPath)) {
        QSettings config(configPath, QSettings::IniFormat);
        config.beginGroup("Appearance");
        colorSchemePath = config.value("ColorScheme", colorSchemePath).toString();
        backgroundImagePath = config.value("BackgroundImage", "").toString();
        config.endGroup();

        qDebug() << "BackgroundImage from config:" << backgroundImagePath;
    } else {
        qWarning() << "Config file not found at:" << configPath;
        if (configPath != "/etc/qmlgreet/qmlgreet.conf") {
            qWarning() << "Using default config path";
        }
    }

    // Load the color scheme and get the palette
    QPalette palette;
    if (QFile::exists(colorSchemePath)) {
        palette = colorScheme->loadColorScheme(colorSchemePath);
        app.setPalette(palette);
        qDebug() << "=== Application Palette Set ===";
    } else {
        qWarning() << "Color scheme not found at:" << colorSchemePath;
        qWarning() << "Using built-in defaults";
    }

    // Set background image if specified and exists
    if (!backgroundImagePath.isEmpty()) {
        if (QFile::exists(backgroundImagePath)) {
            colorScheme->setBackgroundImage(backgroundImagePath);
        } else {
            qWarning() << "Background image not found at:" << backgroundImagePath;
        }
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
