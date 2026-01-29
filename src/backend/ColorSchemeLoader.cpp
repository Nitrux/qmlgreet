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

    // Parse the KDE color scheme file manually
    QMap<QString, QMap<QString, QString>> sections;
    QString currentSection;

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();

        // Skip empty lines and comments
        if (line.isEmpty() || line.startsWith('#') || line.startsWith(';')) {
            continue;
        }

        // Check for section header
        if (line.startsWith('[') && line.endsWith(']')) {
            currentSection = line.mid(1, line.length() - 2);
            continue;
        }

        // Parse key=value pairs
        int equalPos = line.indexOf('=');
        if (equalPos > 0 && !currentSection.isEmpty()) {
            QString key = line.left(equalPos).trimmed();
            QString value = line.mid(equalPos + 1).trimmed();
            sections[currentSection][key] = value;
        }
    }

    file.close();

    // Helper lambda to read color values from parsed sections
    auto readColor = [&](const QString &section, const QString &key) -> QColor {
        if (!sections.contains(section)) {
            qWarning() << "Section not found:" << section;
            return QColor(0, 0, 0);
        }

        if (!sections[section].contains(key)) {
            qWarning() << "Key not found:" << key << "in section" << section;
            return QColor(0, 0, 0);
        }

        QString value = sections[section][key];

        // Parse "R,G,B" or "R, G, B" format
        QStringList parts = value.split(',');
        if (parts.size() == 3) {
            bool ok1, ok2, ok3;
            int r = parts[0].trimmed().toInt(&ok1);
            int g = parts[1].trimmed().toInt(&ok2);
            int b = parts[2].trimmed().toInt(&ok3);

            if (ok1 && ok2 && ok3) {
                QColor color(r, g, b);
                qDebug() << "  " << section << "/" << key << "=" << value << "→" << color.name();
                return color;
            }
        }

        qWarning() << "Failed to parse color from" << section << key << ":" << value;
        return QColor(0, 0, 0);
    };

    // Read colors from KDE color scheme format

    // Colors:View - for background
    QColor viewBackground = readColor("Colors:View", "BackgroundNormal");

    // Colors:Window - for window background and text
    QColor windowBackground = readColor("Colors:Window", "BackgroundNormal");
    QColor windowForeground = readColor("Colors:Window", "ForegroundNormal");

    // Colors:Button - for button styling
    m_buttonBackground = readColor("Colors:Button", "BackgroundNormal");
    m_buttonForeground = readColor("Colors:Button", "ForegroundNormal");
    m_buttonHover = readColor("Colors:Button", "DecorationHover");
    m_buttonFocus = readColor("Colors:Button", "DecorationFocus");

    // Set up the palette for all color groups (Active, Inactive, Disabled)
    for (auto group : {QPalette::Active, QPalette::Inactive}) {
        // Window colors
        palette.setColor(group, QPalette::Window, windowBackground);
        palette.setColor(group, QPalette::WindowText, windowForeground);

        // Base/View colors (for text input fields, list views, etc.)
        palette.setColor(group, QPalette::Base, viewBackground);
        palette.setColor(group, QPalette::AlternateBase, viewBackground);
        palette.setColor(group, QPalette::Text, windowForeground);

        // Button colors
        palette.setColor(group, QPalette::Button, m_buttonBackground);
        palette.setColor(group, QPalette::ButtonText, m_buttonForeground);

        // Highlight colors (for focus/selection)
        palette.setColor(group, QPalette::Highlight, m_buttonFocus);
        palette.setColor(group, QPalette::HighlightedText, m_buttonForeground);

        // Link colors
        palette.setColor(group, QPalette::Link, m_buttonFocus);
        palette.setColor(group, QPalette::LinkVisited, m_buttonFocus);
    }

    // Disabled state
    palette.setColor(QPalette::Disabled, QPalette::Window, windowBackground);
    palette.setColor(QPalette::Disabled, QPalette::WindowText, windowForeground.darker(150));
    palette.setColor(QPalette::Disabled, QPalette::Base, viewBackground);
    palette.setColor(QPalette::Disabled, QPalette::AlternateBase, viewBackground);
    palette.setColor(QPalette::Disabled, QPalette::Text, windowForeground.darker(150));
    palette.setColor(QPalette::Disabled, QPalette::Button, m_buttonBackground.darker(110));
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, m_buttonForeground.darker(150));
    palette.setColor(QPalette::Disabled, QPalette::Highlight, m_buttonFocus.darker(150));
    palette.setColor(QPalette::Disabled, QPalette::HighlightedText, m_buttonForeground.darker(150));

    qDebug() << "Color scheme loaded successfully";
    qDebug() << "  View Background:" << viewBackground.name();
    qDebug() << "  Window Background:" << windowBackground.name();
    qDebug() << "  Button Background:" << m_buttonBackground.name();
    qDebug() << "  Button Hover:" << m_buttonHover.name();
    qDebug() << "  Button Focus:" << m_buttonFocus.name();

    emit colorsChanged();

    return palette;
}
