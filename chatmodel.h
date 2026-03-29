#ifndef CHATMODEL_H
#define CHATMODEL_H

#include <QAbstractListModel> // 【修改】这里改为 QAbstractListModel
#include <QList>
#include <QString>

struct ChatMessage {
    QString name;
    QString content;
    bool isMine;
    QString timestamp; // 新增：时间戳
};

class chatmodel : public QAbstractListModel // 【修改】继承 QAbstractListModel
{
    Q_OBJECT

public:
    explicit chatmodel(QObject *parent = nullptr);
    enum ChatRoles {
        NameRole = Qt::UserRole + 1,
        ContentRole,
        IsMineRole ,
        TimestampRole // 新增：时间角色
    };

    void appendList(const QList<ChatMessage> &messages);
    Q_INVOKABLE void append(const QString &name, const QString &content, bool isMine,const QString &timestamp);
    void clear();
    // 【修改】只需要重写这两个函数即可，大大简化代码
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

private:
    QList<ChatMessage> m_data;
};

#endif // CHATMODEL_H
