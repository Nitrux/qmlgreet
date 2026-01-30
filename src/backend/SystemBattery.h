#pragma once
#include <QObject>
#include <QTimer>

class SystemBattery : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString info READ info NOTIFY infoChanged)
    Q_PROPERTY(bool available READ available NOTIFY availableChanged)

public:
    explicit SystemBattery(QObject *parent = nullptr);

    QString info() const { return m_info; }
    bool available() const { return m_available; }

signals:
    void infoChanged();
    void availableChanged();

private slots:
    void refresh();

private:
    QTimer *m_timer;
    QString m_info;
    bool m_available = false;
};
