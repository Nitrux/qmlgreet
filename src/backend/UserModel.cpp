#include "UserModel.h"
#include <pwd.h>
#include <QFile>
#include <QTextStream>

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
            
            // Look for a face icon (.face or .face.icon)
            QString icon = home + "/.face"; 
            if (!QFile::exists(icon)) icon = "";

            m_users.append({name, gecos.isEmpty() ? name : gecos, icon});
        }
    }
    endpwent();
    
    endResetModel();
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
