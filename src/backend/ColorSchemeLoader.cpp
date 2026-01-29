#include "ColorSchemeLoader.h"
#include <QSettings>
#include <QFile>
#include <QTextStream>
#include <QDebug>

ColorSchemeLoader::ColorSchemeLoader(QObject *parent)
    : QObject(parent)
    , m_backgroundImage("")
{
}

void ColorSchemeLoader::setBackgroundImage(const QString &path)
{
    if (m_backgroundImage != path) {
        m_backgroundImage = path;
        emit backgroundImageChanged();
        qDebug() << "Background image set to:" << path;
    }
}

QPalette ColorSchemeLoader::loadColorScheme(const QString &filePath)
{
    QPalette palette;
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open color scheme file:" << filePath;
        return palette;
    }

    qDebug() << "Loading color scheme from:" << filePath;

    QMap<QString, QMap<QString, QString>> sections;
    QString currentSection;

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#') || line.startsWith(';')) continue;

        if (line.startsWith('[') && line.endsWith(']')) {
            currentSection = line.mid(1, line.length() - 2);
            continue;
        }

        int equalPos = line.indexOf('=');
        if (equalPos > 0 && !currentSection.isEmpty()) {
            QString key = line.left(equalPos).trimmed();
            QString value = line.mid(equalPos + 1).trimmed();
            sections[currentSection][key] = value;
        }
    }
    file.close();

    auto readColor = [&](const QString &section, const QString &key) -> QColor {
        if (!sections.contains(section) || !sections[section].contains(key)) {
            return QColor(0, 0, 0);
        }
        QString value = sections[section][key];
        QStringList parts = value.split(',');
        if (parts.size() == 3) {
            bool ok1, ok2, ok3;
            int r = parts[0].trimmed().toInt(&ok1);
            int g = parts[1].trimmed().toInt(&ok2);
            int b = parts[2].trimmed().toInt(&ok3);
            if (ok1 && ok2 && ok3) return QColor(r, g, b);
        }
        return QColor(0, 0, 0);
    };

    // [MODIFIED] Store to member variables so QML can see them
    m_viewBackground = readColor("Colors:View", "BackgroundNormal");
    m_windowBackground = readColor("Colors:Window", "BackgroundNormal");
    QColor windowForeground = readColor("Colors:Window", "ForegroundNormal");

    m_buttonBackground = readColor("Colors:Button", "BackgroundNormal");
    m_buttonForeground = readColor("Colors:Button", "ForegroundNormal");
    m_buttonHover = readColor("Colors:Button", "DecorationHover");
    m_buttonFocus = readColor("Colors:Button", "DecorationFocus");

    // Standard Qt Palette setup (kept for fallback)
    for (auto group : {QPalette::Active, QPalette::Inactive}) {
        palette.setColor(group, QPalette::Window, m_windowBackground);
        palette.setColor(group, QPalette::WindowText, windowForeground);
        palette.setColor(group, QPalette::Base, m_viewBackground);
        palette.setColor(group, QPalette::AlternateBase, m_viewBackground);
        palette.setColor(group, QPalette::Text, windowForeground);
        palette.setColor(group, QPalette::Button, m_buttonBackground);
        palette.setColor(group, QPalette::ButtonText, m_buttonForeground);
        palette.setColor(group, QPalette::Highlight, m_buttonFocus);
        palette.setColor(group, QPalette::HighlightedText, m_buttonForeground);
    }

    // Emit change signal so bindings update
    emit colorsChanged();

    return palette;
}
