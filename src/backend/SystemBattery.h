#pragma once
#include <QObject>
#include <QTimer>

class SystemBattery : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString info READ info NOTIFY infoChanged)
    Q_PROPERTY(QString iconName READ iconName NOTIFY infoChanged)
    Q_PROPERTY(bool available READ available NOTIFY availableChanged)
    Q_PROPERTY(bool debugBattery READ debugBattery WRITE setDebugBattery NOTIFY debugBatteryChanged)

public:
    explicit SystemBattery(QObject *parent = nullptr);

    QString info() const { return m_info; }
    QString iconName() const { return m_iconName; }
    bool available() const { return m_available; }
    bool debugBattery() const { return m_debugBattery; }
    void setDebugBattery(bool debugBattery);

signals:
    void infoChanged();
    void availableChanged();
    void debugBatteryChanged();

private slots:
    void refresh();

private:
    QTimer *m_timer;
    QString m_info;
    QString m_iconName = QStringLiteral("battery-full");
    bool m_available = false;
    bool m_debugBattery = false;
    int m_debugState = 0;
};
