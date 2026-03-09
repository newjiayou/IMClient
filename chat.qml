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
            chatModel.append(sender, message, isMe)
            // 這裡不再需要手動調用滾動，交由 ListView 的 onCountChanged 處理
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
            chatModel.append(history[i].sender, history[i].message, history[i].isMe);
        }

        // 初始加載完成後強制滾動到底部
        Qt.callLater(listView.positionViewAtEnd);

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
            model: chatModel
            spacing: 10
            clip: true
            topMargin: 10
            bottomMargin: 10

            // 【新增核心邏輯】：當消息行數變化時，延遲執行滾動到底部
            // Qt.callLater 確保在 Delegate 渲染完成後才執行滾動，防止跳轉不到位的問題
            onCountChanged: Qt.callLater(listView.positionViewAtEnd)

            delegate: Item {
                width: listView.width
                height: bubble.height + 25

                // 聊天氣泡
                Rectangle {
                    id: bubble
                    width: Math.min(msgText.implicitWidth + 24, listView.width * 0.7)
                    height: msgText.implicitHeight + 20
                    radius: 10
                    color: model.isMe ? "#95EC69" : "#FFFFFF"

                    anchors.right: model.isMe ? parent.right : undefined
                    anchors.left: model.isMe ? undefined : parent.left
                    anchors.rightMargin: 10
                    anchors.leftMargin: 10

                    Text {
                        id: msgText
                        text: model.messageContent
                        wrapMode: Text.Wrap
                        anchors.fill: parent
                        anchors.margins: 10
                        font.pixelSize: 15
                    }
                }

                // 發送者名字顯示
                Text {
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
            Layout.preferredHeight: 110
            color: "#FFFFFF"
            border.color: "#E0E0E0"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 8

                // 第一行：发送对象
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Text {
                        text: "发送至:"
                        font.pixelSize: 14
                        color: "#666666"
                    }

                    TextField {
                        id: targetField
                        Layout.fillWidth: true
                        Layout.preferredHeight: 35
                        placeholderText: "输入用户名 (如: broadcast)"
                        font.pixelSize: 14
                        background: Rectangle {
                            color: "#F9F9F9"
                            radius: 5
                            border.color: targetField.activeFocus ? "#95EC69" : "#E0E0E0"
                        }
                    }
                }

                // 第二行：消息输入和发送按钮
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    TextField {
                        id: inputField
                        Layout.fillWidth: true
                        Layout.preferredHeight: 40
                        placeholderText: "輸入消息..."
                        font.pixelSize: 15
                        background: Rectangle {
                            color: "#F5F5F5"
                            radius: 5
                        }
                        Keys.onReturnPressed: sendBtn.clicked()
                    }

                    Button {
                        id: sendBtn
                        text: "發送"
                        Layout.preferredWidth: 80
                        Layout.preferredHeight: 40

                        // 只有消息和接收者都不为空时按钮才可用
                        enabled: inputField.text.trim() !== "" && targetField.text.trim() !== ""

                        contentItem: Text {
                            text: sendBtn.text
                            font.bold: true
                            color: "white"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            opacity: sendBtn.enabled ? 1.0 : 0.5
                        }

                        background: Rectangle {
                            color: sendBtn.enabled ? "#07C160" : "#CCCCCC"
                            radius: 5
                        }

                        onClicked: {
                            let target = targetField.text.trim();
                            chatClient.sendMessage(inputField.text.trim(), target);
                            inputField.text = ""
                        }
                    }
                }
            }
        }
    }
}
