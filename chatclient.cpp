#include "ChatClient.h"
#include "BackendWorker.h"   // 引入 Worker
#include "DatabaseManager.h" // 引入 MessageRecord 结构体定义
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QDir>
#include <QDataStream>
#include <QNetworkProxy>
#include <QDateTime>

ChatClient::ChatClient(QObject *parent) : QObject(parent), m_socket(new QTcpSocket(this))
{
    m_chatModel = new chatmodel(this);
    m_socket->setProxy(QNetworkProxy::NoProxy);
    m_networkManager = new QNetworkAccessManager(this);
    // 1. 初始化各类定时器
    m_currentReconnectDelay = 1000;
    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setSingleShot(true);
    connect(m_reconnectTimer, &QTimer::timeout, this, &ChatClient::attemptReconnection);

    m_heartbeatTimer = new QTimer(this);
    m_heartbeatTimeoutTimer = new QTimer(this);
    m_heartbeatTimeoutTimer->setSingleShot(true);
    connect(m_heartbeatTimer, &QTimer::timeout, this, &ChatClient::sendHeartbeat);
    connect(m_heartbeatTimeoutTimer, &QTimer::timeout, this, &ChatClient::onHeartbeatTimeout);

    // 2. 绑定 Socket 信号
    connect(m_socket, &QAbstractSocket::errorOccurred, this, &ChatClient::onErrorOccurred);
    connect(m_socket, &QTcpSocket::readyRead, this, &ChatClient::onReadyRead);
    connect(m_socket, &QTcpSocket::connected, this, &ChatClient::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &ChatClient::onDisconnected);

    // ==========================================
    // 3. 核心：初始化多线程架构
    // ==========================================
    m_workerThread = new QThread(this);
    m_worker = new BackendWorker();

    // 【关键科学步骤】将 Worker 对象的执行上下文强行转移到子线程
    m_worker->moveToThread(m_workerThread);

    // 绑定：主线程发指令 -> 子线程执行 (Qt底层自动排队，深拷贝参数)
    connect(this, &ChatClient::processNetworkBuffer, m_worker, &BackendWorker::onProcessNetworkBuffer);
    connect(this, &ChatClient::initializeDatabase, m_worker, &BackendWorker::onInitializeDatabase);
    // connect(this, &ChatClient::saveOutgoingMessage, m_worker, &BackendWorker::onSaveOutgoingMessage);
    connect(this, &ChatClient::loadInitialHistory, m_worker, &BackendWorker::onLoadInitialHistory);

    // 绑定：子线程出结果 -> 主线程更新UI (Qt底层自动切回主线程执行这些槽函数)
    connect(m_worker, &BackendWorker::loginResult, this, &ChatClient::onLoginResult);
    connect(m_worker, &BackendWorker::historyLoaded, this, &ChatClient::onHistoryLoaded);
    connect(m_worker, &BackendWorker::newMessageReceived, this, &ChatClient::onNewMessageReceived);
    connect(m_worker, &BackendWorker::requestIncrementalHistory, this, &ChatClient::onRequestIncrementalHistory);
    connect(m_worker, &BackendWorker::serverHeartbeat, this, [this](){
        // qDebug() << "❤️[主线程] 收到心跳回包信号，重置超时定时器。";
        if(m_socket->state() == QAbstractSocket::ConnectedState) m_heartbeatTimeoutTimer->start(30000);
    });
    connect(m_worker, &BackendWorker::friendListUpdated, this, [this](const QStringList &list){
        m_friendList = list;
        emit friendListChanged();
    });
    connect(m_worker, &BackendWorker::friendActionResults, this, &ChatClient::friendActionResults);
    // 确保线程退出时，Worker 内存被安全释放
    connect(m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);

    // 启动物理线程，开启事件循环
    m_workerThread->start();
    qDebug() << "[主线程] 后台工作线程已成功启动。";
}

ChatClient::~ChatClient() {
    saveSettings();

    // 安全退出多线程
    if(m_workerThread->isRunning()) {
        m_workerThread->quit(); // 告诉子线程事件循环：准备下班
        m_workerThread->wait(); // 主线程阻塞等待，直到子线程真正完全退出，防止内存泄漏或崩溃
    }
}

void ChatClient::loadConfigAndConnect()
{
    QSettings settings("MyChatApp", "QmlChatClient");
    QString serverIp = settings.value("server/ip", "192.168.56.101").toString();
    quint16 serverPort = static_cast<quint16>(settings.value("server/port", 12346).toUInt());
    connectToServer(serverIp, serverPort);
}

QString ChatClient::currentUser() const { return m_currentUser; }

void ChatClient::setCurrentUser(const QString &user) {
    if (m_currentUser != user) {
        m_currentUser = user;
        emit currentUserChanged();
    }
}

void ChatClient::connectToServer(const QString &ip, quint16 port) {
    m_serverIp = ip;
    m_serverPort = port;

    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->abort();
        m_socket->waitForDisconnected(100);
    }

    qDebug() << "正在连接至服务器:" << ip << ":" << port;
    m_socket->connectToHost(ip, port);
}

void ChatClient::sendMessage(const QString &message, const QString &target) {
    if (m_socket->state() != QAbstractSocket::ConnectedState) return;

    // 1. 发送网络数据包包 (主线程负责网络)
    QJsonObject json;
    json["sender"] = m_currentUser;
    json["message"] = message;
    json["target"] = target;
    m_socket->write(ProtocolHandler::pack(ProtocolHandler::MsgChat, json));


}

void ChatClient::sendidentify(const QString &username, const QString &password)
{
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        m_tempLoginUser = username;
        QJsonObject json;
        json["username"] = username;
        json["password"] = password;
        m_socket->write(ProtocolHandler::pack(ProtocolHandler::MsgLoginReq, json));
    }
}

void ChatClient::addFriend(const QString &name)
{
    if (m_socket->state() != QAbstractSocket::ConnectedState) return;
    QJsonObject json;
    json["friend"] = name;
    m_socket->write(ProtocolHandler::pack(ProtocolHandler::MsgAddFriendReq, json));
}

void ChatClient::onReadyRead() {
    // 【科学架构体现】
    // 主线程只做“搬运工”，读出二进制字节流后，立刻触发跨线程信号。
    // 解包、JSON反序列化、业务逻辑判断，全部由子线程的 Worker 处理，保护 UI 绝对不卡！
    QByteArray rawData = m_socket->readAll();
    emit processNetworkBuffer(rawData, m_currentUser);
}

// ==========================================
// 跨线程回调：工作线程处理完后的 UI 响应
// ==========================================

void ChatClient::onLoginResult(bool success, const QString &message) {
    if (success) {
        setCurrentUser(m_tempLoginUser);
        // 登录成功后，下发指令让子线程去初始化此用户的专属数据库并加载历史
        emit initializeDatabase(m_currentUser);

    }
    // 通知 QML 跳转页面或提示错误
    emit loginResultReceived(success, message);
}

void ChatClient::onHistoryLoaded(const QList<MessageRecord> &history) {
    qDebug() << "[主线程] 接收到历史记录，数量：" << history.size() << "，准备批量更新UI。";

    QList<ChatMessage> newMessages;
    for (const auto &msg : history) {
        newMessages.append({msg.sender, msg.message, (msg.sender == m_currentUser),msg.timestamp});
    }

    m_chatModel->clear();
    m_chatModel->appendList(newMessages); // 瞬间完成，UI 绝对不卡！
}
void ChatClient::onNewMessageReceived(const QString &sender, const QString &message, bool isMe, const QString &timestamp) {
    // 子线程解析到别人的消息并入库后，通知我们画到屏幕上
    if (isMe || sender == m_currentChatTarget || m_currentChatTarget == "broadcast") {
        qDebug() <<("打印转发");
        m_chatModel->append(sender, message, isMe,timestamp);
    }
}

void ChatClient::onRequestIncrementalHistory(const QString &lastTimestamp) {
    // 子线程查出本地最后一条记录的时间后，通知主线程向服务器要增量
    QJsonObject json;
    json["last_timestamp"] = lastTimestamp;
    m_socket->write(ProtocolHandler::pack(ProtocolHandler::MsgHistoryReq, json));
}

void ChatClient::setCurrentChatTarget(const QString &target)
{
    m_chatModel->clear(); // 先清空上一个人的聊天内容
    m_currentChatTarget=target ;
    // 通知 Worker 去加载这个人的历史记录
    // 这里的具体实现取决于你的信号槽，建议发个信号给 Worker
    emit loadInitialHistory(m_currentUser, target);
}

void ChatClient::registerAccount(const QString &username, const QString &password) {
    // 1. 准备请求 (指向 Nginx 监听的 80 端口)
    QUrl url("http://192.168.56.101/api/register"); // 换成你的虚拟机IP
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // 2. 构建 JSON 数据
    QJsonObject json;
    json["username"] = username;
    json["password"] = password;
    QJsonDocument doc(json);
    QByteArray data = doc.toJson();

    // 3. 发送 POST 请求
    QNetworkReply *reply = m_networkManager->post(request, data);

    // 4. 处理响应
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            // 状态码 200 表示成功
            emit registerResultReceived(true, "注册成功！");
        } else {
            // 处理具体错误
            int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (statusCode == 409) {
                emit registerResultReceived(false, "用户名已存在");
            } else {
                emit registerResultReceived(false, "连接服务器失败");
            }
        }
        reply->deleteLater();
    });
}


// ==========================================
// Socket 状态维持与基础重连逻辑
// ==========================================

void ChatClient::onConnected() {
    emit connectionStatusChanged(true);
    if (m_reconnectTimer->isActive()) m_reconnectTimer->stop();

    m_currentReconnectDelay = 1000;
    qDebug() << "✅ 连线成功！心跳机制已启动。";

    m_heartbeatTimer->start(10000);
    m_heartbeatTimeoutTimer->start(30000);
}

void ChatClient::onDisconnected() {
    emit connectionStatusChanged(false);
    m_heartbeatTimer->stop();
    m_heartbeatTimeoutTimer->stop();

    if (!m_reconnectTimer->isActive()) {
        qDebug() << "❌ 连接断开，将尝试重新连线...";
        m_reconnectTimer->start(m_currentReconnectDelay);
    }
}

void ChatClient::attemptReconnection()
{
    qDebug() << "🔄 正在尝试重新连线...";
    if (m_socket->state() != QAbstractSocket::UnconnectedState) m_socket->abort();
    connectToServer(m_serverIp, m_serverPort);

    // 指数退让算法
    m_currentReconnectDelay *= 2;
    if (m_currentReconnectDelay > MAX_RECONNECT_DELAY) {
        m_currentReconnectDelay = MAX_RECONNECT_DELAY;
    }
}

void ChatClient::sendHeartbeat()
{
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->write(ProtocolHandler::pack(ProtocolHandler::MsgHeartbeat));
    }
}

void ChatClient::onHeartbeatTimeout()
{
    m_socket->abort();
}

void ChatClient::onErrorOccurred(QAbstractSocket::SocketError socketError) {
    qDebug() << "❌ Socket 错误代码:" << socketError << " | 原因:" << m_socket->errorString();
    if (socketError == QAbstractSocket::ConnectionRefusedError ||
        socketError == QAbstractSocket::HostNotFoundError ||
        socketError == QAbstractSocket::RemoteHostClosedError)
    {
        if (!m_reconnectTimer->isActive()) {
            m_reconnectTimer->start(m_currentReconnectDelay);
        }
    }
}

void ChatClient::saveSettings()
{
    QSettings settings("MyChatApp", "QmlChatClient");
    settings.setValue("server/ip", m_serverIp);
    settings.setValue("server/port", m_serverPort);
}

void ChatClient::loadSettings()
{
    QSettings settings("MyChatApp", "QmlChatClient");
    QString lastUser = settings.value("lastUser", "").toString();
    setCurrentUser(lastUser);
}