#ifndef COLORSCHEMELOADER_H
#define COLORSCHEMELOADER_H

#include <QObject>
#include <QColor>

class ColorSchemeLoader : public QObject
{
    Q_OBJECT

    // Color properties exposed to QML
    Q_PROPERTY(QColor background READ background NOTIFY colorsChanged)
    Q_PROPERTY(QColor backgroundAlt READ backgroundAlt NOTIFY colorsChanged)
    Q_PROPERTY(QColor textColor READ textColor NOTIFY colorsChanged)
    Q_PROPERTY(QColor primary READ primary NOTIFY colorsChanged)
    Q_PROPERTY(QColor highlight READ highlight NOTIFY colorsChanged)
    Q_PROPERTY(QColor highlightedText READ highlightedText NOTIFY colorsChanged)
    Q_PROPERTY(QColor surface READ surface NOTIFY colorsChanged)
    Q_PROPERTY(QColor surfaceAlt READ surfaceAlt NOTIFY colorsChanged)
    Q_PROPERTY(QColor border READ border NOTIFY colorsChanged)
    Q_PROPERTY(QColor hover READ hover NOTIFY colorsChanged)
    Q_PROPERTY(QColor positive READ positive NOTIFY colorsChanged)
    Q_PROPERTY(QColor negative READ negative NOTIFY colorsChanged)
    Q_PROPERTY(QColor warning READ warning NOTIFY colorsChanged)

public:
    explicit ColorSchemeLoader(QObject *parent = nullptr);

    // Invokable method to load a color scheme file
    Q_INVOKABLE bool loadColorScheme(const QString &filePath);

    // Getters for color properties
    QColor background() const { return m_background; }
    QColor backgroundAlt() const { return m_backgroundAlt; }
    QColor textColor() const { return m_textColor; }
    QColor primary() const { return m_primary; }
    QColor highlight() const { return m_highlight; }
    QColor highlightedText() const { return m_highlightedText; }
    QColor surface() const { return m_surface; }
    QColor surfaceAlt() const { return m_surfaceAlt; }
    QColor border() const { return m_border; }
    QColor hover() const { return m_hover; }
    QColor positive() const { return m_positive; }
    QColor negative() const { return m_negative; }
    QColor warning() const { return m_warning; }

signals:
    void colorsChanged();

private:
    QColor parseRgbString(const QString &rgb);

    // Color storage
    QColor m_background;
    QColor m_backgroundAlt;
    QColor m_textColor;
    QColor m_primary;
    QColor m_highlight;
    QColor m_highlightedText;
    QColor m_surface;
    QColor m_surfaceAlt;
    QColor m_border;
    QColor m_hover;
    QColor m_positive;
    QColor m_negative;
    QColor m_warning;
};

#endif // COLORSCHEMELOADER_H
