#include "ChatClient.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QDir>
#include <QDataStream>

ChatClient::ChatClient(QObject *parent) : QObject(parent), m_socket(new QTcpSocket(this))
{
    initDatabase();
    m_chatModel = new chatmodel(this);  // 创建 model 实例

    // 1. 初始化重連計時器
    m_currentReconnectDelay = 1000; // 初始重连延迟设为 1 秒 (1000ms)
    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setSingleShot(true); // 设为单次触发，由我们手动控制下一次触发的时间
    connect(m_reconnectTimer, &QTimer::timeout, this, &ChatClient::attemptReconnection);
    m_heartbeatTimer = new QTimer(this);
    m_heartbeatTimeoutTimer = new QTimer(this);
    m_heartbeatTimeoutTimer->setSingleShot(true); // 设为单次触发，每次收到数据时重置它
    connect(m_heartbeatTimer, &QTimer::timeout, this, &ChatClient::sendHeartbeat);
    connect(m_heartbeatTimeoutTimer, &QTimer::timeout, this, &ChatClient::onHeartbeatTimeout);

    // ✅ 新增：监听错误信号
    connect(m_socket, &QAbstractSocket::errorOccurred, this, &ChatClient::onErrorOccurred);
    connect(m_socket, &QTcpSocket::readyRead, this, &ChatClient::onReadyRead);
    connect(m_socket, &QTcpSocket::connected, this, &ChatClient::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &ChatClient::onDisconnected);

    // loadSettings();
}

ChatClient::~ChatClient() {
    saveSettings();
    m_db.close();
}

QString ChatClient::currentUser() const { return m_currentUser; }

void ChatClient::setCurrentUser(const QString &user) {
    if (m_currentUser != user) {
        m_currentUser = user;
        emit currentUserChanged();
    }
}

void ChatClient::initDatabase()
{
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName("chathistory.db"); // 存储在运行目录
    if (m_db.open()) {
        QSqlQuery query;
        // 创建消息表
        query.exec("CREATE TABLE IF NOT EXISTS messages ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                   "sender TEXT, "
                   "target TEXT, "
                   "message TEXT, "
                   "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP)");
    } else {
        qDebug() << "Database Error:" << m_db.lastError().text();
    }
}

void ChatClient::connectToServer(const QString &ip, quint16 port) {
    m_serverIp = ip;
    m_serverPort = port;
    if (m_socket->state() != QAbstractSocket::ConnectedState) {
        m_socket->connectToHost(ip, port);
    }
}

void ChatClient::sendMessage(const QString &message,const QString &target) {
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        QJsonObject json;
        json["sender"] = m_currentUser;
        json["message"] = message;
        json["target"]=target;
        // 使用换行符分割 JSON 消息，防止粘包
        QByteArray data = QJsonDocument(json).toJson(QJsonDocument::Compact) ;
        QByteArray packet;
        QDataStream out(&packet, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_6_2); // 确保版本一致

        quint16 messageType =1 ;//消息类型
        quint32 totalLength=sizeof(quint32)+sizeof(quint16)+ data.size();
        out<<totalLength;
        out<<messageType;
        packet.append(data)   ;

        m_socket->write(packet);

        // 保存并显示自己的消息
        m_chatModel->append(m_currentUser, message, true);
        saveMessageToDb(m_currentUser,target,message);
    }
}

void ChatClient::onReadyRead() {
    m_buffer.append(m_socket->readAll());

    while (true) {
        if (m_buffer.size() < sizeof(quint32)) break;

        QDataStream in(m_buffer);
        in.setVersion(QDataStream::Qt_6_2);

        quint32 totalLength;
        in >> totalLength; // 读了 4 字节，指针在 4

        if (m_buffer.size() < totalLength) break;

        // --- 修正：直接读类型，不要重复读长度 ---
        quint16 messageType;
        in >> messageType; // 读了 2 字节，指针在 6

        // 截取消息体：从第 6 个字节开始，长度是 总长 - 6
        QByteArray messageBody = m_buffer.mid(6, totalLength - 6);

        if (messageType == 1) {
            QJsonDocument doc = QJsonDocument::fromJson(messageBody);
            if (!doc.isNull()) {
                QJsonObject json = doc.object();
                QString sender = json["sender"].toString();
                QString message = json["message"].toString();
                QString target = json["target"].toString();

                if (sender != m_currentUser) {
                    if (target == "broadcast" || target == m_currentUser) {
                        m_chatModel->append(sender, message, false);
                    }
                }
            }
        } else if (messageType == 2) {
            qDebug() << "收到心跳回包";
        }

        // 处理完一个完整的包，从缓冲区移除
        m_buffer.remove(0, totalLength);
    }
}

void ChatClient::saveMessageToDb(const QString &sender, const QString &target,const QString &message) {
    QSqlQuery query;
    query.prepare("INSERT INTO messages (sender, target, message) VALUES (:sender, :target, :message)");
    query.bindValue(":sender", sender);
    query.bindValue(":target", target);
    query.bindValue(":message", message);
    query.exec();
}

void ChatClient::saveSettings()
{
    QSettings settings("MyChatApp", "QmlChatClient");

    // 将当前的用户名以 "lastUser" 为键，保存起来
    settings.setValue("lastUser", m_currentUser);

    qDebug() << "保存配置：已保存当前用户 -> " << m_currentUser;
}

void ChatClient::loadSettings()
{
    QSettings settings("MyChatApp", "QmlChatClient");

    // 读取 "lastUser" 这个键。如果键不存在（比如第一次运行），则返回一个空字符串 ""
    QString lastUser = settings.value("lastUser", "").toString();

    // 将读取到的用户名设置给 m_currentUser
    // 注意：这里我们不再需要 main.cpp 里的随机用户名了
    setCurrentUser(lastUser);

    qDebug() << "加载配置：上次登录的用户是 -> " << m_currentUser;
}

QVariantList ChatClient::loadHistory() {
    QVariantList history;
    QSqlQuery query("SELECT sender, target, message FROM messages ORDER BY timestamp ASC");
    while (query.next()) {
        QVariantMap map;
        QString sender = query.value(0).toString();
        QString target = query.value(1).toString();
        QString message = query.value(2).toString();

        map["sender"] = sender;
        map["target"] = target; // ✅ 将 target 传给 QML
        map["message"] = message;
        map["isMe"] = (sender == m_currentUser);
        history.append(map);
    }
    return history;
}

void ChatClient::loadHistoryToModel()
{
    if (!m_chatModel) return;

    QSqlQuery query("SELECT sender, message FROM messages ORDER BY timestamp ASC");
    while (query.next()) {
        QString sender = query.value(0).toString();
        QString message = query.value(1).toString();
        bool isMe = (sender == m_currentUser);

        // 使用 model 添加历史消息
        m_chatModel->append(sender, message, isMe);
    }
}

void ChatClient::onConnected() {
    emit connectionStatusChanged(true);
    if (m_reconnectTimer->isActive()) {
        m_reconnectTimer->stop();
    }

    QByteArray packet;
    QDataStream out(&packet, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_6_2);

    QJsonObject json;
    json["sender"] = m_currentUser;
    json["message"] = ""; // 或者是你要发的其他信息
    json["target"] = "";

    // 1. 先生成 JSON 数据
    QByteArray data = QJsonDocument(json).toJson(QJsonDocument::Compact);

    // 2. 正确计算总长度：长度字段(4) + 类型字段(2) + 数据体长度
    quint16 messageType = 3;
    quint32 totalLength = sizeof(quint32) + sizeof(quint16) + data.size(); // 必须用 data.size()

    // 3. 写入二进制头部
    out << totalLength;
    out << messageType;

    // 4. 将 JSON 数据追加到包体
    packet.append(data);

    // 5. 发送
    m_socket->write(packet);

    m_socket->write(packet);
    m_currentReconnectDelay = 1000; // 重置为 1 秒
    qDebug() << "✅ 連線成功！重連延遲已重置為初始值。";

    // ✅ 新增：连接成功后，启动心跳机制
    m_heartbeatTimer->start(10000);        // 每 10 秒发送一次心跳
    m_heartbeatTimeoutTimer->start(30000); // 启动 30 秒的超时倒计时
    qDebug() << "❤️ 心跳机制已启动。";
}
void ChatClient::onDisconnected() {
    emit connectionStatusChanged(false);
    m_heartbeatTimer->stop();
    m_heartbeatTimeoutTimer->stop();

    if (!m_reconnectTimer->isActive()) {
        qDebug() << "❌ 連接斷開，將在" << m_currentReconnectDelay / 1000.0 << "秒後嘗試重新連線...";
        m_reconnectTimer->start(m_currentReconnectDelay);
    }
}

void ChatClient::attemptReconnection()
{
    qDebug() << "🔄 正在嘗試重新連線...";

    // ✅ 关键：如果 Socket 还卡在上次的连接状态中，先强制中断它，保证每次尝试都是干净的
    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->abort();
    }

    // 发起连接
    connectToServer(m_serverIp, m_serverPort);

    // ✅ 指数退让算法：将下一次的重连延迟时间翻倍（* 2）
    m_currentReconnectDelay *= 2;

    // ✅ 封顶限制：不能让它无限增大，最大不超过 MAX_RECONNECT_DELAY (60秒)
    if (m_currentReconnectDelay > MAX_RECONNECT_DELAY) {
        m_currentReconnectDelay = MAX_RECONNECT_DELAY;
    }

    // ✅ 启动下一次的重连定时器。
    // 如果刚才的 connectToServer 成功了，onConnected() 会把这个定时器停掉。
    // 如果一直没连上，这个定时器就会在新的、更长的时间后再次触发本函数。
    qDebug() << "如果本次失敗，下次將在" << m_currentReconnectDelay / 1000.0 << "秒後重試...";
    m_reconnectTimer->start(m_currentReconnectDelay);
}

void ChatClient::sendHeartbeat()
{
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        QByteArray packet;
        QDataStream out(&packet, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_6_2);

        quint16 messageType = 2; // 2 代表心跳包
        // 心跳包没有数据体，所以长度只有 长度字段本身 + 消息类型字段
        quint32 totalLength = sizeof(quint32) + sizeof(quint16);
        out << totalLength;
        out << messageType;

        m_socket->write(packet);
        // qDebug() << "已发送心跳 Ping..."; // 可选：打印日志
    }
}

void ChatClient::onHeartbeatTimeout()
{
    qDebug() << "❌ 心跳超时！长时间未收到服务器数据，判定为死链接，准备断开重连...";
    // abort() 会立即强制关闭 Socket，并触发 disconnected 信号
    // 这将自动引导程序进入你已有的 onDisconnected() 重连逻辑
    m_socket->abort();
}

void ChatClient::onErrorOccurred(QAbstractSocket::SocketError socketError) {
    qDebug() << "❌ Socket 错误代码:" << socketError;

    // 只有在连接相关错误时，才启动重连
    if (socketError == QAbstractSocket::ConnectionRefusedError ||
        socketError == QAbstractSocket::HostNotFoundError ||
        socketError == QAbstractSocket::RemoteHostClosedError)
    {
        // 关键：如果重连定时器没在跑，就启动它！
        if (!m_reconnectTimer->isActive()) {
            qDebug() << "准备启动重连逻辑...";
            m_reconnectTimer->start(m_currentReconnectDelay);
        }
    }
}
