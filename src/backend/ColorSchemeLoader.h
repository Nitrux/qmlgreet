#ifndef COLORSCHEMELOADER_H
#define COLORSCHEMELOADER_H

#include <QObject>
#include <QColor>
#include <QPalette>

class ColorSchemeLoader : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString backgroundImage READ backgroundImage NOTIFY backgroundImageChanged)

    // [NEW] Added missing background properties
    Q_PROPERTY(QColor viewBackground READ viewBackground NOTIFY colorsChanged)
    Q_PROPERTY(QColor windowBackground READ windowBackground NOTIFY colorsChanged)

    Q_PROPERTY(QColor buttonBackground READ buttonBackground NOTIFY colorsChanged)
    Q_PROPERTY(QColor buttonForeground READ buttonForeground NOTIFY colorsChanged)
    Q_PROPERTY(QColor buttonHover READ buttonHover NOTIFY colorsChanged)
    Q_PROPERTY(QColor buttonFocus READ buttonFocus NOTIFY colorsChanged)

public:
    explicit ColorSchemeLoader(QObject *parent = nullptr);

    Q_INVOKABLE QPalette loadColorScheme(const QString &filePath);

    QString backgroundImage() const { return m_backgroundImage; }

    // [NEW] Getters
    QColor viewBackground() const { return m_viewBackground; }
    QColor windowBackground() const { return m_windowBackground; }

    QColor buttonBackground() const { return m_buttonBackground; }
    QColor buttonForeground() const { return m_buttonForeground; }
    QColor buttonHover() const { return m_buttonHover; }
    QColor buttonFocus() const { return m_buttonFocus; }

    void setBackgroundImage(const QString &path);

signals:
    void backgroundImageChanged();
    void colorsChanged();

private:
    QString m_backgroundImage;
    
    // [NEW] Member variables
    QColor m_viewBackground;
    QColor m_windowBackground;
    
    QColor m_buttonBackground;
    QColor m_buttonForeground;
    QColor m_buttonHover;
    QColor m_buttonFocus;
};

#endif // COLORSCHEMELOADER_H
