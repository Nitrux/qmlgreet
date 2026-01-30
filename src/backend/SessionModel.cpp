#include "SessionModel.h"
#include <QDir>
#include <QSettings>
#include <QStandardPaths>
#include <QDebug>

SessionModel::SessionModel(QObject *parent) : QAbstractListModel(parent) {
    refresh();
}

void SessionModel::refresh() {
    beginResetModel();
    m_sessions.clear();

    // Follow XDG Base Directory specification for session files
    // Check XDG_DATA_DIRS (defaults to /usr/local/share:/usr/share if not set)
    QString dataDirs = qEnvironmentVariable("XDG_DATA_DIRS", "/usr/local/share:/usr/share");
    QStringList searchPaths = dataDirs.split(':', Qt::SkipEmptyParts);

    qDebug() << "SessionModel: Searching for sessions in:" << searchPaths;

    // Load from each directory in XDG_DATA_DIRS
    for (const QString &baseDir : searchPaths) {
        QString sessionPath = baseDir + "/wayland-sessions";
        if (QDir(sessionPath).exists()) {
            qDebug() << "SessionModel: Loading sessions from:" << sessionPath;
            loadSessionsFrom(sessionPath, "wayland");
        }
    }

    qDebug() << "SessionModel: Total sessions loaded:" << m_sessions.count();
    for (int i = 0; i < m_sessions.count(); ++i) {
        qDebug() << "  [" << i << "]" << m_sessions[i].name << ":" << m_sessions[i].exec;
    }

    endResetModel();
}

void SessionModel::loadSessionsFrom(const QString &path, const QString &type) {
    QDir dir(path);
    dir.setNameFilters(QStringList() << "*.desktop");
    
    for (const QString &filename : dir.entryList()) {
        QSettings desktopFile(dir.absoluteFilePath(filename), QSettings::IniFormat);
        desktopFile.beginGroup("Desktop Entry");
        
        QString name = desktopFile.value("Name").toString();
        QString exec = desktopFile.value("Exec").toString();
        
        // Hide hidden sessions
        if (desktopFile.value("NoDisplay", false).toBool() || desktopFile.value("Hidden", false).toBool())
            continue;

        if (!name.isEmpty() && !exec.isEmpty()) {
            m_sessions.append({name, exec, type});
        }
    }
}

int SessionModel::rowCount(const QModelIndex &) const {
    return m_sessions.count();
}

QVariant SessionModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= m_sessions.count())
        return QVariant();

    const Session &session = m_sessions[index.row()];
    switch (role) {
        case NameRole: return session.name;
        case ExecRole: return session.exec;
        case TypeRole: return session.type;
        default: return QVariant();
    }
}

QHash<int, QByteArray> SessionModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[ExecRole] = "exec";
    roles[TypeRole] = "type";
    return roles;
}

QString SessionModel::execCommand(int index) {
    qDebug() << "SessionModel: execCommand called with index:" << index << "/ total sessions:" << m_sessions.count();

    if (index >= 0 && index < m_sessions.count()) {
        QString cmd = m_sessions[index].exec;
        qDebug() << "SessionModel: Returning command:" << cmd;
        return cmd;
    }

    qWarning() << "SessionModel: Invalid index, returning empty string";
    return QString();
}
