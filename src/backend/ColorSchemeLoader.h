#ifndef COLORSCHEMELOADER_H
#define COLORSCHEMELOADER_H

#include <QObject>
#include <QColor>
#include <QPalette>

class ColorSchemeLoader : public QObject
{
    Q_OBJECT

    // Background image property
    Q_PROPERTY(QString backgroundImage READ backgroundImage NOTIFY backgroundImageChanged)

    // Button color properties for QML
    Q_PROPERTY(QColor buttonBackground READ buttonBackground NOTIFY colorsChanged)
    Q_PROPERTY(QColor buttonForeground READ buttonForeground NOTIFY colorsChanged)
    Q_PROPERTY(QColor buttonHover READ buttonHover NOTIFY colorsChanged)
    Q_PROPERTY(QColor buttonFocus READ buttonFocus NOTIFY colorsChanged)

public:
    explicit ColorSchemeLoader(QObject *parent = nullptr);

    // Invokable method to load a color scheme file and return a QPalette
    Q_INVOKABLE QPalette loadColorScheme(const QString &filePath);

    // Getter for background image
    QString backgroundImage() const { return m_backgroundImage; }

    // Getters for button colors
    QColor buttonBackground() const { return m_buttonBackground; }
    QColor buttonForeground() const { return m_buttonForeground; }
    QColor buttonHover() const { return m_buttonHover; }
    QColor buttonFocus() const { return m_buttonFocus; }

    // Setter for background image
    void setBackgroundImage(const QString &path);

signals:
    void backgroundImageChanged();
    void colorsChanged();

private:
    QString m_backgroundImage;
    QColor m_buttonBackground;
    QColor m_buttonForeground;
    QColor m_buttonHover;
    QColor m_buttonFocus;
};

#endif // COLORSCHEMELOADER_H
