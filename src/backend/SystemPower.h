#pragma once
#include <QObject>

class SystemPower : public QObject
{
    Q_OBJECT
public:
    explicit SystemPower(QObject *parent = nullptr);

    Q_INVOKABLE void powerOff();
    Q_INVOKABLE void reboot();
    Q_INVOKABLE void suspend();
    Q_INVOKABLE void hibernate();
    Q_INVOKABLE void hybridSleep();
    Q_INVOKABLE void suspendThenHibernate();

    // Check if actions are available
    Q_INVOKABLE bool canPowerOff();
    Q_INVOKABLE bool canReboot();
    Q_INVOKABLE bool canSuspend();
    Q_INVOKABLE bool canHibernate();
    Q_INVOKABLE bool canHybridSleep();
    Q_INVOKABLE bool canSuspendThenHibernate();
};
