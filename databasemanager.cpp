#include "DatabaseManager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QDateTime>

DatabaseManager::DatabaseManager(QObject *parent) : QObject(parent) {}

DatabaseManager::~DatabaseManager() {
    if (m_db.isOpen()) m_db.close();
}

bool DatabaseManager::openDatabase(const QString &username) {
    // 防止重复打开同一个连接名
    m_currentConnName = QString("Conn_%1").arg(username);

    if (QSqlDatabase::contains(m_currentConnName)) {
        m_db = QSqlDatabase::database(m_currentConnName);
    } else {
        m_db = QSqlDatabase::addDatabase("QSQLITE", m_currentConnName);
    }

    m_db.setDatabaseName(QString("history_%1.db").arg(username));

    if (!m_db.open()) {
        qDebug() << "无法打开数据库:" << m_db.lastError().text();
        return false;
    }

    QSqlQuery query(m_db);
    query.exec("CREATE TABLE IF NOT EXISTS friends ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT, "
               "friend_name TEXT UNIQUE)"); // UNIQUE 防止重复添加
    return query.exec("CREATE TABLE IF NOT EXISTS messages ("
                      "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                      "sender TEXT, target TEXT, message TEXT, "
                      "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP)");
}

void DatabaseManager::saveMessage(const QString &sender, const QString &target,
                                  const QString &message, const QString &timestamp) {
    if (!m_db.isOpen()) return;

    QSqlQuery query(m_db);
    query.prepare("INSERT INTO messages (sender, target, message, timestamp) "
                  "VALUES (:sender, :target, :message, :timestamp)");
    query.bindValue(":sender", sender);
    query.bindValue(":target", target);
    query.bindValue(":message", message);
    query.bindValue(":timestamp", timestamp);

    if (!query.exec()) {
        qDebug() << "保存消息失败:" << query.lastError().text();
    }
}

QString DatabaseManager::getLastTimestamp() {
    QSqlQuery query("SELECT timestamp FROM messages ORDER BY timestamp DESC LIMIT 1", m_db);
    if (query.next()) {
        return query.value(0).toString();
    }
    return "1970-01-01 00:00:00";
}
bool DatabaseManager::transaction() {
    return m_db.transaction();
}

bool DatabaseManager::commit() {
    return m_db.commit();
}
QList<MessageRecord> DatabaseManager::getAllHistory() {
    QList<MessageRecord> history;
    QSqlQuery query("SELECT sender, target, message, timestamp FROM messages ORDER BY timestamp ASC", m_db);

    while (query.next()) {
        history.append({
            query.value(0).toString(),
            query.value(1).toString(),
            query.value(2).toString(),
            query.value(3).toString()
        });
    }
    return history;
}

void DatabaseManager::addFriend(const QString &name)
{
    QSqlQuery query(m_db);
    query.prepare("INSERT OR IGNORE INTO friends (friend_name) VALUES (:name)");
    query.bindValue(":name", name);
    query.exec();
}

QStringList DatabaseManager::getFriends()
{
    QStringList list;
    QSqlQuery query("SELECT friend_name FROM friends", m_db);
    while (query.next()) {
        list << query.value(0).toString();
    }
    return list;
}

QList<MessageRecord> DatabaseManager::getHistoryForTarget(const QString &myId, const QString &targetId)
{
    QList<MessageRecord> history;
    QSqlQuery query(m_db);

    // 关键：只查“我发给对方”或“对方发给我”的消息
    QString sql = QString(
                      "SELECT sender, target, message, timestamp FROM messages "
                      "WHERE (sender='%1' AND target='%2') OR (sender='%2' AND target='%1') "
                      "ORDER BY timestamp ASC"
                      ).arg(myId).arg(targetId);

    if (query.exec(sql)) {
        while (query.next()) {
            history.append({query.value(0).toString(), query.value(1).toString(),
                            query.value(2).toString(), query.value(3).toString()});
        }
    }
    return history;
}