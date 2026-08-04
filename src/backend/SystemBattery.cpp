#include "SystemBattery.h"
#include <QDir>
#include <QFile>
#include <QDebug>

SystemBattery::SystemBattery(QObject *parent) : QObject(parent)
{
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &SystemBattery::refresh);
    m_timer->start(10000); // Check every 10 seconds
    refresh();
}

void SystemBattery::setDebugBattery(bool debugBattery)
{
    if (m_debugBattery == debugBattery) {
        return;
    }

    m_debugBattery = debugBattery;
    m_debugState = 0;
    m_timer->setInterval(m_debugBattery ? 2000 : 10000);
    emit debugBatteryChanged();
    refresh();
}

void SystemBattery::refresh()
{
    if (m_debugBattery) {
        const int debugPercentages[] = { 0, 25, 50, 75, 100 };
        const bool charging = m_debugState >= 5;
        const int percent = debugPercentages[m_debugState % 5];
        const QString status = charging ? QStringLiteral("Charging")
                                        : QStringLiteral("Discharging");
        const QString level = percent < 10 ? QStringLiteral("caution")
            : percent < 30 ? QStringLiteral("low")
            : percent < 80 ? QStringLiteral("good") : QStringLiteral("full");
        const QString newIconName = charging
            ? QStringLiteral("battery-%1-charging").arg(level)
            : QStringLiteral("battery-%1").arg(level);
        const QString newInfo = QStringLiteral("%1% (%2)").arg(percent).arg(status);
        const bool wasAvailable = m_available;

        m_info = newInfo;
        m_iconName = newIconName;
        m_available = true;
        m_debugState = (m_debugState + 1) % 10;

        if (!wasAvailable) {
            emit availableChanged();
        }
        emit infoChanged();
        return;
    }

    QString batteryPath;
    QDir dir("/sys/class/power_supply");
    
    // Loop to find type "Battery"
    for (const QString &entry : dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        QString path = dir.absoluteFilePath(entry);
        QFile typeFile(path + "/type");
        if (typeFile.open(QIODevice::ReadOnly)) {
            QString type = QString::fromUtf8(typeFile.readAll()).trimmed();
            if (type == "Battery") {
                batteryPath = path;
                break;
            }
        }
    }

    // No battery found
    if (batteryPath.isEmpty()) {
        if (!m_info.isEmpty()) {
            m_info.clear();
            emit infoChanged();
        }
        if (m_available) {
            m_available = false;
            emit availableChanged();
        }
        return;
    }

    // Read Capacity
    QFile capFile(batteryPath + "/capacity");
    QString capacity = "0";
    if (capFile.open(QIODevice::ReadOnly)) {
        capacity = QString::fromUtf8(capFile.readAll()).trimmed();
    }

    // Read Status
    QFile statFile(batteryPath + "/status");
    QString status = "Unknown";
    if (statFile.open(QIODevice::ReadOnly)) {
        status = QString::fromUtf8(statFile.readAll()).trimmed();
    }

    const int percent = capacity.toInt();
    const QString level = percent < 10 ? QStringLiteral("caution")
        : percent < 30 ? QStringLiteral("low")
        : percent < 80 ? QStringLiteral("good") : QStringLiteral("full");
    const bool charging = status == QStringLiteral("Charging")
        || status == QStringLiteral("Full");
    const QString newIconName = charging
        ? QStringLiteral("battery-%1-charging").arg(level)
        : QStringLiteral("battery-%1").arg(level);
    QString newInfo = QString("%1% (%2)").arg(capacity, status);

    if (m_info != newInfo || m_iconName != newIconName) {
        m_info = newInfo;
        m_iconName = newIconName;
        emit infoChanged();
    }

    if (!m_available) {
        m_available = true;
        emit availableChanged();
    }
}
