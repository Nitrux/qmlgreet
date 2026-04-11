#pragma once
#include <QAbstractListModel>

struct User {
    QString username;
    QString realName;
    QString iconPath;
};

class UserModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum UserRoles {
        UsernameRole = Qt::UserRole + 1,
        RealNameRole,
        IconRole
    };

    explicit UserModel(QObject *parent = nullptr);
    explicit UserModel(const QString &avatarOverridePattern, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

private:
    void loadUsers();
    QString findUserAvatar(const QString &username, const QString &homeDir) const;
    QString resolveAvatarOverride(const QString &username, const QString &homeDir) const;
    bool isUsableAvatarFile(const QString &path) const;

    QString m_avatarOverridePattern;
    QVector<User> m_users;
};
