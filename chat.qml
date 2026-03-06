import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Page {
    id: rootWindow
    title: "QML Chat - " + chatClient.currentUser
    // 用於連接 C++ 信號
    Connections {
        target: chatClient

        function onMessageReceived(sender, message, isMe) {
            // 修正：ChatModel.append() 需要三個獨立參數，而不是一個對象
            chatModel.append(sender, message, isMe)
            listView.positionViewAtEnd() // 滾動到最底部
        }

        function onConnectionStatusChanged(connected) {
            statusText.text = connected ? "已連接" : "未連接"
            statusText.color = connected ? "green" : "red"
        }
    }

    Component.onCompleted: {
        // 加載歷史記錄
        let history = chatClient.loadHistory();
        for (let i = 0; i < history.length; i++) {
            // 修正：ChatModel.append() 需要三個獨立參數
            chatModel.append(history[i].sender, history[i].message, history[i].isMe);
        }
        listView.positionViewAtEnd();

        // 自動連接到本地服務器
        chatClient.connectToServer("127.0.0.1", 12345);
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // 頂部狀態欄
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            color: "#ECECEC"

            Text {
                id: statusText
                anchors.centerIn: parent
                text: "正在連接..."
                color: "orange"
                font.pixelSize: 14
            }
        }

        // 聊天記錄展示區
        ListView {
            id: listView
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: chatModel//将model赋值给QML
            spacing: 10
            clip: true
            topMargin: 10
            bottomMargin: 10

            delegate: Item {
                width: listView.width
                height: bubble.height + 20

                // 聊天氣泡
                Rectangle {
                    id: bubble
                    width: Math.min(msgText.implicitWidth + 24, listView.width * 0.7)
                    height: msgText.implicitHeight + 20
                    radius: 10
                    color: model.isMe ? "#95EC69" : "#FFFFFF" // QQ/微信綠和白

                    // 根據是否是自己發的消息，實現左右對齊
                    anchors.right: model.isMe ? parent.right : undefined  //此时model就可以直接用 一个model代表一行数据
                    anchors.left: model.isMe ? undefined : parent.left
                    anchors.rightMargin: 10
                    anchors.leftMargin: 10

                    Text {
                        id: msgText
                        // 注意：這裡使用 model.messageContent 而不是 model.message
                        // 這對應 C++ ChatModel 中 roleNames() 返回的 "messageContent" 角色
                        text: model.messageContent //用model里面的哈希值进行转化后的角色
                        wrapMode: Text.Wrap
                        anchors.fill: parent
                        anchors.margins: 10
                        font.pixelSize: 15
                    }
                }

                // 發送者名字顯示
                Text {
                    // 注意：這裡使用 model.senderName 而不是 model.sender
                    // 這對應 C++ ChatModel 中 roleNames() 返回的 "senderName" 角色
                    text: model.senderName
                    font.pixelSize: 10
                    color: "gray"
                    anchors.top: bubble.bottom
                    anchors.topMargin: 2
                    anchors.right: model.isMe ? parent.right : undefined
                    anchors.left: model.isMe ? undefined : parent.left
                    anchors.rightMargin: 12
                    anchors.leftMargin: 12
                }
            }
        }

        // 底部輸入區
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            color: "#FFFFFF"
            border.color: "#E0E0E0"

            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 10

                TextField {
                    id: inputField
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    placeholderText: "輸入消息..."
                    font.pixelSize: 15
                    background: Rectangle {
                        color: "#F5F5F5"
                        radius: 5
                    }
                    Keys.onReturnPressed: sendBtn.clicked() // 回車發送
                }

                Button {
                    id: sendBtn
                    text: "發送"
                    Layout.preferredWidth: 80
                    Layout.fillHeight: true
                    onClicked: {
                        if (inputField.text.trim() !== "") {
                            chatClient.sendMessage(inputField.text.trim())
                            inputField.text = ""
                        }
                    }
                }
            }
        }
    }
}
