#include "UserModel.h"
#include <pwd.h>
#include <QFile>

UserModel::UserModel(QObject *parent) : QAbstractListModel(parent) {
    beginResetModel();

    // Iterate system users
    struct passwd *pwent;
    setpwent();
    while ((pwent = getpwent()) != NULL) {
        int uid = pwent->pw_uid;

        // Filter: standard users usually start at 1000.
        if (uid >= 1000 && uid < 60000) {
            QString name = pwent->pw_name;
            QString gecos = QString::fromUtf8(pwent->pw_gecos).split(",").first();
            QString home = pwent->pw_dir;

            // Try multiple sources for user avatar, in order of preference:
            // 1. AccountsService (system-wide, readable by greeter)
            // 2. ~/.face (user's home, may not be readable by _greetd)
            QString icon = findUserAvatar(name, home);

            m_users.append({name, gecos.isEmpty() ? name : gecos, icon});
        }
    }
    endpwent();

    endResetModel();
}

QString UserModel::findUserAvatar(const QString &username, const QString &homeDir) {
    // Try standard locations in order of preference

    // 1. Try ~/.face
    QString icon = homeDir + "/.face";
    if (QFile::exists(icon)) {
        return icon;
    }

    // 2. Try ~/.face.icon
    icon = homeDir + "/.face.icon";
    if (QFile::exists(icon)) {
        return icon;
    }

    // 3. Try AccountsService (readable by system greeter)
    icon = QString("/var/lib/AccountsService/icons/%1").arg(username);
    if (QFile::exists(icon)) {
        return icon;
    }

    // 4. Use bundled fallback icon
    return "qrc:/icons/user-avatar.svg";
}

int UserModel::rowCount(const QModelIndex &) const {
    return m_users.count();
}

QVariant UserModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= m_users.count())
        return QVariant();

    const User &user = m_users[index.row()];
    switch (role) {
        case UsernameRole: return user.username;
        case RealNameRole: return user.realName;
        case IconRole: return user.iconPath;
        default: return QVariant();
    }
}

QHash<int, QByteArray> UserModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[UsernameRole] = "username";
    roles[RealNameRole] = "realName";
    roles[IconRole] = "iconPath";
    return roles;
}
