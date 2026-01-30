#include "SystemPower.h"
#include <QDBusInterface>
#include <QDBusReply>
#include <QDebug>

SystemPower::SystemPower(QObject *parent) : QObject(parent) {}

void SystemPower::powerOff()
{
    QDBusInterface interface("org.freedesktop.login1", 
                             "/org/freedesktop/login1",
                             "org.freedesktop.login1.Manager", 
                             QDBusConnection::systemBus());
    
    // Call PowerOff(interactive_boolean). true = let other apps prompt to save
    interface.call("PowerOff", true);
}

void SystemPower::reboot()
{
    QDBusInterface interface("org.freedesktop.login1", 
                             "/org/freedesktop/login1",
                             "org.freedesktop.login1.Manager", 
                             QDBusConnection::systemBus());
    interface.call("Reboot", true);
}

void SystemPower::suspend()
{
    QDBusInterface interface("org.freedesktop.login1", 
                             "/org/freedesktop/login1",
                             "org.freedesktop.login1.Manager", 
                             QDBusConnection::systemBus());
    interface.call("Suspend", true);
}

void SystemPower::hibernate()
{
    QDBusInterface interface("org.freedesktop.login1",
                             "/org/freedesktop/login1",
                             "org.freedesktop.login1.Manager",
                             QDBusConnection::systemBus());
    interface.call("Hibernate", true);
}

void SystemPower::hybridSleep()
{
    QDBusInterface interface("org.freedesktop.login1",
                             "/org/freedesktop/login1",
                             "org.freedesktop.login1.Manager",
                             QDBusConnection::systemBus());
    interface.call("HybridSleep", true);
}

void SystemPower::suspendThenHibernate()
{
    QDBusInterface interface("org.freedesktop.login1",
                             "/org/freedesktop/login1",
                             "org.freedesktop.login1.Manager",
                             QDBusConnection::systemBus());
    interface.call("SuspendThenHibernate", true);
}

bool SystemPower::canPowerOff()
{
    QDBusInterface interface("org.freedesktop.login1",
                             "/org/freedesktop/login1",
                             "org.freedesktop.login1.Manager",
                             QDBusConnection::systemBus());
    QDBusReply<QString> reply = interface.call("CanPowerOff");
    return reply.isValid() && (reply.value() == "yes");
}

bool SystemPower::canReboot()
{
    QDBusInterface interface("org.freedesktop.login1",
                             "/org/freedesktop/login1",
                             "org.freedesktop.login1.Manager",
                             QDBusConnection::systemBus());
    QDBusReply<QString> reply = interface.call("CanReboot");
    return reply.isValid() && (reply.value() == "yes");
}

bool SystemPower::canSuspend()
{
    QDBusInterface interface("org.freedesktop.login1",
                             "/org/freedesktop/login1",
                             "org.freedesktop.login1.Manager",
                             QDBusConnection::systemBus());
    QDBusReply<QString> reply = interface.call("CanSuspend");
    return reply.isValid() && (reply.value() == "yes");
}

bool SystemPower::canHibernate()
{
    QDBusInterface interface("org.freedesktop.login1",
                             "/org/freedesktop/login1",
                             "org.freedesktop.login1.Manager",
                             QDBusConnection::systemBus());
    QDBusReply<QString> reply = interface.call("CanHibernate");
    return reply.isValid() && (reply.value() == "yes");
}

bool SystemPower::canHybridSleep()
{
    QDBusInterface interface("org.freedesktop.login1",
                             "/org/freedesktop/login1",
                             "org.freedesktop.login1.Manager",
                             QDBusConnection::systemBus());
    QDBusReply<QString> reply = interface.call("CanHybridSleep");
    return reply.isValid() && (reply.value() == "yes");
}

bool SystemPower::canSuspendThenHibernate()
{
    QDBusInterface interface("org.freedesktop.login1",
                             "/org/freedesktop/login1",
                             "org.freedesktop.login1.Manager",
                             QDBusConnection::systemBus());
    QDBusReply<QString> reply = interface.call("CanSuspendThenHibernate");
    return reply.isValid() && (reply.value() == "yes");
}
