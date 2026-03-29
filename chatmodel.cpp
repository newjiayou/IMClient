#include "chatmodel.h"

chatmodel::chatmodel(QObject *parent)
    : QAbstractListModel(parent) // 【修改】对应这里的父类构造
{}

void chatmodel::appendList(const QList<ChatMessage> &messages)
{
    if (messages.isEmpty()) return;

    // 一次性通知视图我们要插入 N 行，QML 只会渲染一次！
    beginInsertRows(QModelIndex(), m_data.count(), m_data.count() + messages.count() - 1);
    m_data.append(messages);
    endInsertRows();
}
void chatmodel::append(const QString &name, const QString &content, bool isMine, const QString &timestamp)
{
    beginInsertRows(QModelIndex(), m_data.count(), m_data.count());
    m_data.append({name, content, isMine,timestamp});
    endInsertRows();
}

void chatmodel::clear()
{
    beginResetModel();

    // 清空你底层存储数据的容器（假设你叫 m_messages）
    m_data.clear();

    // 【核心】通知视图：重置完成，你可以重新渲染为空白了
    endResetModel();
}

int chatmodel::rowCount(const QModelIndex &parent) const
{
    // 【修改】如果是子节点，返回0行（因为我们是平面列表）；如果是根节点，返回实际行数
    if (parent.isValid())
        return 0;
    return m_data.count();
}

QVariant chatmodel::data(const QModelIndex &index, int role) const
{
    // 【修改】加上越界保护，防止崩溃
    if (!index.isValid() || index.row() >= m_data.count() || index.row() < 0)
        return QVariant();

    const ChatMessage &msg = m_data.at(index.row());
    switch(role)
    {
    case NameRole:
        return msg.name;
    case ContentRole:
        return msg.content;
    case IsMineRole:
        return msg.isMine;
    case TimestampRole:     // 新增：向 QML 返回时间
        return msg.timestamp;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> chatmodel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[NameRole] = "senderName";
    roles[ContentRole] = "messageContent";
    roles[IsMineRole] = "isMe";
    roles[TimestampRole] = "timestamp";
    return roles;
}
