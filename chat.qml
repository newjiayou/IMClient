import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Page {
    id: chatDetailPage

    // 【核心新增】定义属性接收从上一页传过来的好友名
    property string chatTarget: ""

    // 修改标题显示
    title: "正在与 " + chatTarget + " 聊天"

    // 头部工具栏：增加返回按钮
    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            ToolButton {
                text: "‹ 返回"
                onClicked: chatDetailPage.StackView.view.pop()
            }
            Label {
                text: chatTarget
                elide: Label.ElideRight
                verticalAlignment: Qt.AlignVCenter
                horizontalAlignment: Qt.AlignHCenter
                Layout.fillWidth: true
                font.bold: true
            }
            Item { Layout.preferredWidth: 40 } // 占位保持标题居中
        }
    }

    // 【修改】加上 timestamp 参数匹配后端的信号
    function onNewMessageReceived(sender, message, timestamp, isMe) {
        // 逻辑判断：只显示当前聊天的对象，或者广播，或者我自己发出的
        if (isMe || sender === chatTarget || chatTarget === "broadcast") {
            chatModel.append(sender, message, timestamp, isMe)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        ListView {
            id: listView
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: chatModel
            spacing: 15
            clip: true

            // 【核心修改】将原本的 delegate 替换为纵向排列的 Column，以容纳时间和气泡
            delegate: Column {
                width: listView.width
                spacing: 5
                topPadding: 10

                // 1. 时间显示 (居中)
                Text {
                    // 如果模型中有时间则显示，否则留空
                    text: model.timestamp ? model.timestamp : ""
                    color: "#aaaaaa"
                    font.pixelSize: 12
                    anchors.horizontalCenter: parent.horizontalCenter
                    // 如果没有时间，则隐藏且不占据空间
                    visible: text !== ""
                }

                // 2. 消息气泡外层容器 (用于控制左右对齐)
                Item {
                    width: parent.width
                    height: bubble.height

                    Rectangle {
                        id: bubble
                        width: Math.min(msgText.implicitWidth + 24, parent.width * 0.7)
                        height: msgText.implicitHeight + 20
                        radius: 10
                        color: model.isMe ? "#95EC69" : "#FFFFFF"

                        // 根据是否是我自己发的消息，控制气泡靠左还是靠右
                        anchors.right: model.isMe ? parent.right : undefined
                        anchors.left: model.isMe ? undefined : parent.left
                        anchors.rightMargin: model.isMe ? 10 : 0
                        anchors.leftMargin: model.isMe ? 0 : 10

                        Text {
                            id: msgText
                            text: model.messageContent
                            anchors.fill: parent
                            anchors.margins: 10
                            font.pixelSize: 15
                            wrapMode: Text.Wrap
                        }
                    }
                }
            }

            onCountChanged: Qt.callLater(listView.positionViewAtEnd)
        }

        // 底部输入区
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 70
            color: "#FFFFFF"
            border.color: "#E0E0E0"

            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 10

                TextField {
                    id: inputField
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40
                    placeholderText: "輸入消息..."
                    font.pixelSize: 15
                    background: Rectangle { color: "#F5F5F5"; radius: 5 }
                    Keys.onReturnPressed: sendBtn.clicked()
                }

                Button {
                    id: sendBtn
                    text: "發送"
                    Layout.preferredWidth: 80
                    Layout.preferredHeight: 40
                    enabled: inputField.text.trim() !== ""

                    background: Rectangle {
                        color: sendBtn.enabled ? "#07C160" : "#CCCCCC"
                        radius: 5
                    }

                    onClicked: {
                        // 【核心修改】直接使用属性 chatTarget，不再从输入框获取
                        chatClient.sendMessage(inputField.text.trim(), chatTarget);
                        inputField.text = ""
                    }
                }
            }
        }
    }
}