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
#include "backend/AuthWrapper.h"
#include "backend/SessionModel.h"
#include "backend/UserModel.h"
#include "backend/SystemPower.h"
#include "backend/LayerShell.h"
#include "backend/ColorSchemeLoader.h"
#include "backend/SystemBattery.h"

int main(int argc, char *argv[])
{
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

    // Register Types
    qmlRegisterType<AuthWrapper>("QmlGreet", 1, 0, "AuthWrapper");
    qmlRegisterType<SessionModel>("QmlGreet", 1, 0, "SessionModel");
    qmlRegisterType<UserModel>("QmlGreet", 1, 0, "UserModel");
    qmlRegisterType<SystemPower>("QmlGreet", 1, 0, "SystemPower");
    qmlRegisterType<LayerShell>("QmlGreet", 1, 0, "LayerShell");
    qmlRegisterType<SystemBattery>("QmlGreet", 1, 0, "SystemBattery");

    ColorSchemeLoader *colorScheme = new ColorSchemeLoader(&app);

    // Default Configuration
    QString configPath = parser.value(configOption);
    QString colorSchemePath = "/usr/share/color-schemes/Nitrux.colors";
    QString backgroundImagePath;
    QString iconTheme = "Luv"; 
    QString fontName = "Noto Sans";
    int fontSize = 10;

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
    }

    // Set Application Font
    if (!fontName.isEmpty()) {
        QFont font(fontName);
        if (fontSize > 0) {
            font.setPointSize(fontSize);
        }
        app.setFont(font);
    }

    // Set the Icon Theme explicitly
    if (!iconTheme.isEmpty()) {
        QIcon::setThemeName(iconTheme);
    }

    // Load Color Scheme (Palette)
    QPalette palette;
    if (QFile::exists(colorSchemePath)) {
        palette = colorScheme->loadColorScheme(colorSchemePath);
        app.setPalette(palette);
    }

    // Load Background
    if (!backgroundImagePath.isEmpty() && QFile::exists(backgroundImagePath)) {
        colorScheme->setBackgroundImage(backgroundImagePath);
    }

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("ColorScheme", colorScheme);

    const QUrl url(QStringLiteral("qrc:/resources/qml/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl) QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);

    engine.load(url);

    return app.exec();
}
