#include "BackendWorker.h"
#include <QDebug>
#include <QJsonArray>

BackendWorker::BackendWorker(QObject *parent) : QObject(parent)
{
    // 在工作線程中創建數據庫管理器實例
    m_dbManager = new DatabaseManager(this);
}

BackendWorker::~BackendWorker() {}

void BackendWorker::onInitializeDatabase(const QString &username)
{
    qDebug() << "[Worker] 正在為" << username << "初始化數據庫...";
    m_dbManager->openDatabase(username);
    QString lastTime = m_dbManager->getLastTimestamp();
    emit requestIncrementalHistory(lastTime);
}

void BackendWorker::SaveOutgoingMessage(const QString &sender, const QString &target, const QString &message, const QString &timestamp)
{
    qDebug() << "[Worker] 正在將發出的消息保存至數據庫...";
    m_dbManager->saveMessage(sender, target, message, timestamp);
}

void BackendWorker::onLoadInitialHistory(const QString &username,const QString &target)
{
    qDebug() << "[Worker] 正在從數據庫加載全部歷史記錄...";
    QList<MessageRecord> history = m_dbManager->getHistoryForTarget(username,target);
    emit historyLoaded(history); // 加載完成後，發送信號通知主線程

    // 加載完本地歷史後，可以請求增量歷史
}

// 核心功能：解析所有來自服務器的數據包
void BackendWorker::onProcessNetworkBuffer(QByteArray buffer, const QString &currentUser)
{
    m_networkBuffer.append(buffer);

    // 循環解包，直到緩衝區不足一個完整包
    while (auto packet = ProtocolHandler::unpack(m_networkBuffer)) {
        qDebug() << "[Worker] 解包成功，類型:" << packet->type;
        switch (packet->type) {

        case ProtocolHandler::MsgChat: {
            QString sender = packet->body["sender"].toString();
            QString message = packet->body["message"].toString();
            QString target = packet->body["target"].toString();
            QString timestamp = packet->body["timestamp"].toString();
            if (target == "broadcast" || target == currentUser) {
                // 1. 在工作線程保存到數據庫
                m_dbManager->saveMessage(sender, target, message, timestamp);
                qDebug() <<("收到来信开始绘制");
                // 2. 發送信號通知主線程更新UI
                emit newMessageReceived(sender, message, false,timestamp); // isMe=false 表示是接收的消息
            }
            if(sender == currentUser )
            {
                SaveOutgoingMessage(sender,target,message,timestamp);
                qDebug() <<("存入自己写的消息");
            }
            break;
        }

        case ProtocolHandler::MsgLoginRes: {
            QString result = packet->body["result"].toString();
            QString serverMsg = packet->body["message"].toString();
            emit loginResult(result == "success", serverMsg.isEmpty() ? "用戶名或密碼錯誤" : serverMsg);
            break;
        }

        case ProtocolHandler::MsgHistoryRes: {
            QJsonArray historyArray = packet->body["array"].toArray();

            // 1. 开启数据库事务，将接下来的 INSERT 暂存到内存
            m_dbManager->transaction();

            // 用于收集需要发送给 UI 的消息列表
            QList<MessageRecord> newRecords;

            for (const QJsonValue &value : historyArray) {
                QJsonObject msgObj = value.toObject();
                QString sender = msgObj["sender"].toString();
                QString target = msgObj["target"].toString();
                QString content = msgObj["content"].toString();
                QString timestamp = msgObj["timestamp"].toString();

                // 保存到数据库 (此时只是写入缓存)
                m_dbManager->saveMessage(sender, target, content, timestamp);

                // 暂存到列表
                newRecords.append({sender, target, content, timestamp});
            }

            // 2. 批量提交事务 (一次性写入磁盘，速度提升百倍)
            m_dbManager->commit();

            // 3. 复用上面改好的批量加载信号（不要发 N 次 newMessageReceived）
            emit historyLoaded(newRecords);
            break;
        }

        case ProtocolHandler::MsgHeartbeat:
            emit serverHeartbeat();
            break;

        case ProtocolHandler::MsgAddFriendRes: {
            bool success = packet->body["result"].toString() == "success";
            QString friendName = packet->body["friend"].toString();
            QString msg = packet->body["message"].toString();

            if (success) {
                m_dbManager->addFriend(friendName); // 保存到本地
                emit friendListUpdated(m_dbManager->getFriends()); // 通知 UI 更新列表
            }
            emit friendActionResults(success, msg); // 通知 UI 提示信息
            break;
        }

        case ProtocolHandler::MsgFriendListRes: {
            QJsonArray arr = packet->body["friends"].toArray();
            for (const auto &v : arr) {
                m_dbManager->addFriend(v.toString());
            }
            emit friendListUpdated(m_dbManager->getFriends());
            break;
        }




        default:
            qDebug() << "[Worker] 收到未處理的包類型:" << packet->type;
            break;
        }
    }
}