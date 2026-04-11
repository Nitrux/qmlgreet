#include "UserModel.h"
#include <pwd.h>
#include <QFileInfo>
#include <QImageReader>

UserModel::UserModel(QObject *parent)
    : UserModel(QString(), parent)
{
}

UserModel::UserModel(const QString &avatarOverridePattern, QObject *parent)
    : QAbstractListModel(parent)
    , m_avatarOverridePattern(avatarOverridePattern.trimmed())
{
    loadUsers();
}

void UserModel::loadUsers() {
    beginResetModel();
    m_users.clear();

    struct passwd *pwent;
    setpwent();
    while ((pwent = getpwent()) != nullptr) {
        const int uid = pwent->pw_uid;

        if (uid >= 1000 && uid < 60000) {
            const QString name = pwent->pw_name;
            const QString gecos = QString::fromUtf8(pwent->pw_gecos).split(",").first();
            const QString home = pwent->pw_dir;
            const QString icon = findUserAvatar(name, home);

            m_users.append({name, gecos.isEmpty() ? name : gecos, icon});
        }
    }
    endpwent();

    endResetModel();
}

QString UserModel::findUserAvatar(const QString &username, const QString &homeDir) const {
    const QString configuredAvatar = resolveAvatarOverride(username, homeDir);
    if (isUsableAvatarFile(configuredAvatar)) {
        return configuredAvatar;
    }

    QString icon = homeDir + "/.face";
    if (isUsableAvatarFile(icon)) {
        return icon;
    }

    icon = homeDir + "/.face.icon";
    if (isUsableAvatarFile(icon)) {
        return icon;
    }

    icon = QString("/var/lib/AccountsService/icons/%1").arg(username);
    if (isUsableAvatarFile(icon)) {
        return icon;
    }

    return "qrc:/icons/user-avatar.svg";
}

QString UserModel::resolveAvatarOverride(const QString &username, const QString &homeDir) const {
    if (m_avatarOverridePattern.isEmpty()) {
        return QString();
    }

    QString resolvedPath = m_avatarOverridePattern;
    resolvedPath.replace("%u", username);
    resolvedPath.replace("%h", homeDir);
    return resolvedPath;
}

bool UserModel::isUsableAvatarFile(const QString &path) const {
    if (path.isEmpty()) {
        return false;
    }

    if (path.startsWith("qrc:")) {
        return true;
    }

    const QFileInfo info(path);
    if (!info.exists() || !info.isFile() || !info.isReadable()) {
        return false;
    }

    QImageReader reader(path);
    reader.setDecideFormatFromContent(true);
    return reader.canRead();
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
