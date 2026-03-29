#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QStringList>
#include <QVariantMap>

// 定义一个简单的结构体用于导出数据
struct MessageRecord {
    QString sender;
    QString target;
    QString message;
    QString timestamp;
};
class DatabaseManager:public QObject
{
public:
    // 新增开启和提交事务的接口
    bool transaction();
    bool commit();
    explicit DatabaseManager(QObject * parent= nullptr);
    ~DatabaseManager();
    bool openDatabase(const QString &username);
    void saveMessage(const QString &sender, const QString &target,
                     const QString &message, const QString &timestamp);
    QString getLastTimestamp();
    QList<MessageRecord> getAllHistory();
    void addFriend(const QString &name);
    QString currentConnName() const;
    void setCurrentConnName(const QString &newCurrentConnName);
    QStringList getFriends();
    QList<MessageRecord> getHistoryForTarget(const QString &myId, const QString &targetId);
signals:
    void currentConnNameChanged();

private:
    QSqlDatabase m_db;
    QString m_currentConnName;

    Q_PROPERTY(QString currentConnName READ currentConnName WRITE setCurrentConnName NOTIFY currentConnNameChanged FINAL)
};

#endif // DATABASEMANAGER_H
