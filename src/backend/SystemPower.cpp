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
