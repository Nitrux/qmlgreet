#include "ColorSchemeLoader.h"
#include <QSettings>
#include <QDebug>

ColorSchemeLoader::ColorSchemeLoader(QObject *parent)
    : QObject(parent)
{
    // Initialize with default Catppuccin Mocha colors as fallback
    m_background = QColor(30, 30, 46);           // Base
    m_backgroundAlt = QColor(24, 24, 37);        // Mantle
    m_textColor = QColor(205, 214, 244);         // Text
    m_primary = QColor(137, 180, 250);           // Blue
    m_highlight = QColor(137, 180, 250);         // Blue
    m_highlightedText = QColor(30, 30, 46);      // Base (for contrast)
    m_surface = QColor(49, 50, 68);              // Surface0
    m_surfaceAlt = QColor(88, 91, 112);          // Overlay0
    m_border = QColor(69, 71, 90);               // Surface1
    m_hover = QColor(108, 112, 134);             // Overlay1
    m_positive = QColor(166, 227, 161);          // Green
    m_negative = QColor(243, 139, 168);          // Maroon
    m_warning = QColor(249, 226, 175);           // Yellow
}

QColor ColorSchemeLoader::parseRgbString(const QString &rgb)
{
    // KDE color schemes use format: "R,G,B" or "R, G, B"
    QStringList parts = rgb.split(',');
    if (parts.size() != 3) {
        qWarning() << "Invalid RGB string:" << rgb;
        return QColor(255, 0, 255); // Return magenta for invalid colors
    }

    bool ok1, ok2, ok3;
    int r = parts[0].trimmed().toInt(&ok1);
    int g = parts[1].trimmed().toInt(&ok2);
    int b = parts[2].trimmed().toInt(&ok3);

    if (!ok1 || !ok2 || !ok3) {
        qWarning() << "Failed to parse RGB values from:" << rgb;
        return QColor(255, 0, 255);
    }

    return QColor(r, g, b);
}

bool ColorSchemeLoader::loadColorScheme(const QString &filePath)
{
    QSettings settings(filePath, QSettings::IniFormat);

    if (settings.status() != QSettings::NoError) {
        qWarning() << "Failed to load color scheme from:" << filePath;
        return false;
    }

    qDebug() << "Loading color scheme from:" << filePath;

    // Helper lambda to safely read color values with fallback
    auto readColor = [&](const QString &group, const QString &key, const QColor &fallback) -> QColor {
        settings.beginGroup(group);
        QString value = settings.value(key).toString().trimmed();
        settings.endGroup();

        if (value.isEmpty()) {
            return fallback;
        }

        QColor color = parseRgbString(value);
        // If parsing failed (magenta), use fallback
        return (color == QColor(255, 0, 255)) ? fallback : color;
    };

    // Read colors from different sections based on KDE color scheme format

    // Window colors
    m_background = readColor("Colors:Window", "BackgroundNormal", m_background);
    m_backgroundAlt = readColor("Colors:Window", "BackgroundAlternate", m_background);
    m_textColor = readColor("Colors:Window", "ForegroundNormal", m_textColor);

    // Selection colors
    m_highlight = readColor("Colors:Selection", "BackgroundNormal", m_highlight);
    m_highlightedText = readColor("Colors:Selection", "ForegroundNormal", m_highlightedText);

    // View colors (for surfaces)
    m_surface = readColor("Colors:View", "BackgroundNormal", m_surface);
    m_surfaceAlt = readColor("Colors:View", "BackgroundAlternate", m_surface);
    m_border = readColor("Colors:View", "DecorationFocus", m_border);
    m_hover = readColor("Colors:View", "DecorationHover", m_hover);

    // Button colors (for primary)
    m_primary = readColor("Colors:Button", "BackgroundNormal", m_primary);

    // Status colors from Window section (most reliable)
    m_positive = readColor("Colors:Window", "ForegroundPositive", m_positive);
    m_negative = readColor("Colors:Window", "ForegroundNegative", m_negative);
    m_warning = readColor("Colors:Window", "ForegroundNeutral", m_warning);

    emit colorsChanged();
    qDebug() << "Color scheme loaded successfully";
    qDebug() << "  Background:" << m_background.name();
    qDebug() << "  Text:" << m_textColor.name();
    qDebug() << "  Highlight:" << m_highlight.name();

    return true;
}
