#ifndef BACKENDWORKER_H
#define BACKENDWORKER_H

#include <QObject>
#include <QJsonObject>
#include "DatabaseManager.h"
#include "protocolhandler.h"

// BackendWorker 負責所有耗時操作：DB讀寫、JSON解析
class BackendWorker : public QObject
{
    Q_OBJECT
public:
    explicit BackendWorker(QObject *parent = nullptr);
    ~BackendWorker();
    void SaveOutgoingMessage(const QString &sender, const QString &target, const QString &message, const QString &timestamp);
public slots:
    // 從主線程接收的任務
    void onInitializeDatabase(const QString &username);

    void onLoadInitialHistory(const QString &username,const QString &target);
    void onProcessNetworkBuffer(QByteArray buffer, const QString &currentUser);

signals:
    // 將處理結果發送回主線程
    void loginResult(bool success, const QString &message);
    void historyLoaded(const QList<MessageRecord> &history);
    void newMessageReceived(const QString &sender, const QString &message, bool isMe, const QString &timestamp);
    void serverHeartbeat();
    void requestIncrementalHistory(const QString &lastTimestamp);
    void friendListUpdated(const QStringList &friends);
    void friendActionResults(bool success, const QString &message);
private:
    DatabaseManager *m_dbManager; // 每個 Worker 擁有自己的 DB 管理器
    QByteArray m_networkBuffer;   // 網絡數據緩衝區
};

#endif // BACKENDWORKER_H