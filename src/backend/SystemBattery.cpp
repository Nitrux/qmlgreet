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

void SystemBattery::refresh()
{
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

    // Format: 🔋 50% (Discharging)
    QString newInfo = QString("🔋 %1% (%2)").arg(capacity, status);

    if (m_info != newInfo) {
        m_info = newInfo;
        emit infoChanged();
    }

    if (!m_available) {
        m_available = true;
        emit availableChanged();
    }
}
