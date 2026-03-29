#ifndef PROTOCOLHANDLER_H
#define PROTOCOLHANDLER_H

#include <QIODevice>
#include <QDataStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>  // 修复当前错误
#include <QDebug>      // 建议加上，方便调试
class ProtocolHandler
{
public:
    // 1. 定义消息类型枚举，让代码具备可读性
    enum MessageType : quint16 {
        MsgChat       = 1, // 聊天消息
        MsgHeartbeat  = 2, // 心跳包
        MsgLoginReq   = 4, // 登录请求
        MsgLoginRes   = 5, // 登录响应
        MsgHistoryReq = 7, // 历史请求
        MsgHistoryRes = 8 , // 历史响应
        MsgAddFriendReq   = 9,  // 添加好友请求
        MsgAddFriendRes   = 10, // 添加好友结果响应
        MsgFriendListReq  = 11, // 获取好友列表请求
        MsgFriendListRes  = 12  // 好友列表结果响应
    };

    // 定义一个结构体表示解析出来的完整包
    struct Packet {
        MessageType type;
        QJsonObject body;
    };

    // 2. 封包：将 JSON 转换为二进制流
    static QByteArray pack(MessageType type, const QJsonObject &json = QJsonObject());

    // 3. 拆包：尝试从缓冲区提取一个完整的包
    // 使用 std::optional，如果缓冲区不够一个包，返回空
    static std::optional<Packet> unpack(QByteArray &buffer);

private:
    static const int HEADER_SIZE = 6; // 4字节长度 + 2字节类型
};

#endif // PROTOCOLHANDLER_H