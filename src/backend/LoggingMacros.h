#pragma once

#include <QDebug>

// Enable verbose debug logging only in debug builds
// In release builds, these become no-ops
#ifdef QT_NO_DEBUG_OUTPUT
    #define LOG_DEBUG(msg) do {} while (0)
    #define LOG_INFO(msg) qInfo() << msg
#else
    #define LOG_DEBUG(msg) qDebug() << msg
    #define LOG_INFO(msg) qInfo() << msg
#endif

// Always log warnings and errors regardless of build type
#define LOG_WARN(msg) qWarning() << msg
#define LOG_ERROR(msg) qCritical() << msg
