#ifndef CHATCLIENT_H
#define CHATCLIENT_H
#include "chatmodel.h"
#include <QObject>
#include <QTcpSocket>
#include <QSqlDatabase>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVariantList>
#include <QtQmlIntegration>  // 新增：必须包含此头文件以使用 QML_ELEMENT
#include <QTimer> // 1. 引入 QTimer 頭檔案
class ChatClient : public QObject
{
    Q_OBJECT
    QML_ELEMENT  // 现在编译器能识别这个宏了
    Q_PROPERTY(QString currentUser READ currentUser WRITE setCurrentUser NOTIFY currentUserChanged)
    Q_PROPERTY(chatmodel* chatModel READ chatModel CONSTANT)
public:
    explicit ChatClient(QObject *parent = nullptr);
    chatmodel* chatModel() const { return m_chatModel; }
    ~ChatClient();

    QString currentUser() const;
    void setCurrentUser(const QString &user);

    // 暴露给 QML 调用的函数
    Q_INVOKABLE void connectToServer(const QString &ip, quint16 port);
    Q_INVOKABLE void sendMessage(const QString &message);
    Q_INVOKABLE QVariantList loadHistory(); // 加载本地聊天记录
    Q_INVOKABLE void loadHistoryToModel();

signals:
    void currentUserChanged();
    void messageReceived(const QString &sender, const QString &message, bool isMe);
    void connectionStatusChanged(bool connected);

private slots:
    void onReadyRead();
    void onConnected();
    void onDisconnected();
    void attemptReconnection(); // 4. 新增用於重連的槽函數
    // ✅ 新增：心跳相关的槽函数
    void sendHeartbeat();
    void onHeartbeatTimeout();
    void onErrorOccurred(QAbstractSocket::SocketError socketError);
private:
    QByteArray m_buffer;
    void initDatabase();
    void saveMessageToDb(const QString &sender, const QString &message);
    void saveSettings(); // 用于保存配置
    void loadSettings(); // 用于加载配置
    QTcpSocket *m_socket;
    QSqlDatabase m_db;
    QString m_currentUser;
    chatmodel *m_chatModel;
    QTimer *m_reconnectTimer;  //用于重连

    int m_currentReconnectDelay;             // 当前的重连延迟时间（毫秒）
    const int MAX_RECONNECT_DELAY = 60000;   // 最大重连延迟时间（例如 60 秒）

    QTimer *m_heartbeatTimer;        // 定时发送心跳包
    QTimer *m_heartbeatTimeoutTimer; // 检测是否超时未收到数据的定时器
    QString m_serverIp;         //记录存储的IP
    quint16 m_serverPort;   //记录存储端口号

};

#endif // CHATCLIENT_H
