#include "chatmodel.h"

chatmodel::chatmodel(QObject *parent)
    : QAbstractListModel(parent) // 【修改】对应这里的父类构造
{}

void chatmodel::append(const QString &name, const QString &content, bool isMine)
{
    beginInsertRows(QModelIndex(), m_data.count(), m_data.count());
    m_data.append({name, content, isMine});
    endInsertRows();
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
    return roles;
}
