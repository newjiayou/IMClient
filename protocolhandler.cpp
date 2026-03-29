#include "ProtocolHandler.h"

QByteArray ProtocolHandler::pack(MessageType type, const QJsonObject &json)
{
    QByteArray dataBody = QJsonDocument(json).toJson(QJsonDocument::Compact);

    QByteArray packet;
    QDataStream out(&packet, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_6_2);

    // 计算总长度：4(长度) + 2(类型) + 数据体长度
    quint32 totalLength = HEADER_SIZE + dataBody.size();

    // 写入头部
    out << totalLength;
    out << static_cast<quint16>(type);

    // 写入数据体
    if (!dataBody.isEmpty()) {
        packet.append(dataBody);
    }

    return packet;
}

std::optional<ProtocolHandler::Packet> ProtocolHandler::unpack(QByteArray &buffer)
{
    // 检查缓冲区是否至少包含长度字段（4字节）
    if (buffer.size() < 4) return std::nullopt;

    QDataStream in(buffer);
    in.setVersion(QDataStream::Qt_6_2);

    quint32 totalLength;
    in >> totalLength;

    // 检查缓冲区是否包含整个完整包
    if (buffer.size() < static_cast<int>(totalLength)) {
        return std::nullopt;
    }

    // 读取类型（接下来的2字节）
    quint16 typeInt;
    in >> typeInt;

    // 提取 JSON 数据体
    QByteArray bodyData = buffer.mid(HEADER_SIZE, totalLength - HEADER_SIZE);

    // 从缓冲区中移除已经处理完的数据
    buffer.remove(0, totalLength);

    // 组装结果
    Packet packet;
    packet.type = static_cast<MessageType>(typeInt);

    if (!bodyData.isEmpty()) {
        QJsonDocument doc = QJsonDocument::fromJson(bodyData);
        if (doc.isObject()) packet.body = doc.object();
        else if (doc.isArray()) {
            // 处理历史记录是数组的情况，包装进一个 Object
            packet.body.insert("array", doc.array());
        }
    }

    return packet;
}