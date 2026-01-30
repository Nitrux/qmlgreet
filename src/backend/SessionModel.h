#pragma once
#include <QAbstractListModel>

struct Session {
    QString name;
    QString exec;
    QString type;
};

class SessionModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum SessionRoles {
        NameRole = Qt::UserRole + 1,
        ExecRole,
        TypeRole
    };

    explicit SessionModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Call this to reload sessions from disk
    Q_INVOKABLE void refresh();
    
    // Helper to get the command for a specific index
    Q_INVOKABLE QString execCommand(int index);

private:
    void loadSessionsFrom(const QString &path, const QString &type);
    QVector<Session> m_sessions;
};
