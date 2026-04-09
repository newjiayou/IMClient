#ifndef CHATCLIENT_H
#define CHATCLIENT_H

#include "chatmodel.h"
#include <QObject>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVariantList>
#include <QtQmlIntegration>  // 新增：必须包含此头文件以使用 QML_ELEMENT
#include <QTimer>            // 1. 引入 QTimer 頭檔案
#include "protocolhandler.h"
#include <QSettings>         // [新增] 引入QSettings头文件，用于读写配置
#include <QThread>           // [新增] 引入多线程支持
#include <QNetworkAccessManager>
#include <QNetworkReply>

class BackendWorker;         // 前向聲明
struct MessageRecord;        // 前向聲明

class ChatClient : public QObject
{
    Q_OBJECT
    QML_ELEMENT  // 现在编译器能识别这个宏了
    Q_PROPERTY(QString currentUser READ currentUser WRITE setCurrentUser NOTIFY currentUserChanged)
    Q_PROPERTY(chatmodel* chatModel READ chatModel CONSTANT)
    Q_PROPERTY(QStringList friendList READ friendList NOTIFY friendListChanged)
public:
    explicit ChatClient(QObject *parent = nullptr);
    chatmodel* chatModel() const { return m_chatModel; }
    ~ChatClient();

    void loadConfigAndConnect();
    QString currentUser() const;
    void setCurrentUser(const QString &user);

    // 暴露给 QML 调用的函数
    Q_INVOKABLE void connectToServer(const QString &ip, quint16 port);
    Q_INVOKABLE void sendMessage(const QString &message, const QString &target);
    Q_INVOKABLE void sendidentify(const QString &username, const QString &password);
    Q_INVOKABLE void addFriend(const QString &name);
    QStringList friendList() const { return m_friendList; }
    Q_INVOKABLE void setCurrentChatTarget(const QString &target);
    Q_INVOKABLE void registerAccount(const QString &username, const QString &password);

signals:
    void currentUserChanged();
    void connectionStatusChanged(bool connected);
    void loginResultReceived(bool success, const QString &message);

    // 發送給 Worker 線程的信號
    void initializeDatabase(const QString &username);
    void loadInitialHistory(const QString &username,const QString &target);
    void processNetworkBuffer(QByteArray buffer, const QString &currentUser);
    void friendListChanged();
    void friendActionResults(bool success, const QString &message);
    void registerResultReceived(bool success, const QString &message);


private slots:
    void onReadyRead();
    void onConnected();
    void onDisconnected();
    void attemptReconnection();
    void sendHeartbeat();
    void onHeartbeatTimeout();
    void onErrorOccurred(QAbstractSocket::SocketError socketError);

    // 從 Worker 線程接收結果的槽函數
    void onLoginResult(bool success, const QString &message);
    void onHistoryLoaded(const QList<MessageRecord> &history);
    void onNewMessageReceived(const QString &sender, const QString &message, bool isMe, const QString &timestamp);
    void onRequestIncrementalHistory(const QString &lastTimestamp);
private:
    void saveSettings(); // 用于保存配置
    void loadSettings(); // 用于加载配置

    QTcpSocket *m_socket;
    QString m_currentUser;
    QString m_tempLoginUser;   // 正在尝试登录的用户（临时存放）
    chatmodel *m_chatModel;
    QTimer *m_reconnectTimer;  // 用于重连

    int m_currentReconnectDelay;             // 当前的重连延迟时间（毫秒）
    const int MAX_RECONNECT_DELAY = 60000;   // 最大重连延迟时间（例如 60 秒）

    void requestIncrementalHistory();
    QTimer *m_heartbeatTimer;        // 定时发送心跳包
    QTimer *m_heartbeatTimeoutTimer; // 检测是否超时未收到数据的定时器
    QString m_serverIp;              // 记录存储的IP
    quint16 m_serverPort;            // 记录存储端口号

    // --- 多線程成員 ---
    QThread* m_workerThread;
    BackendWorker* m_worker;
    QString m_currentChatTarget; // 必须添加这一行来跟踪当前正在和谁聊
    QNetworkAccessManager *m_networkManager; // 专门处理 HTTP (注册
    QStringList m_friendList;
};

#endif // CHATCLIENT_H