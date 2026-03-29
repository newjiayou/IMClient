import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: mainChatPage
    title: "我的通讯录"

    // 假设 chatClient.currentUser 在 C++ 中已经设置好了
    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            Label {
                text: "当前用户: " + (chatClient.currentUser || "未登录")
                elide: Label.ElideRight
                Layout.leftMargin: 10
                Layout.fillWidth: true
                color: "white"
                font.bold: true
            }
            Button {
                text: "退出"
                onClicked: {
                    // 返回登录界面
                    if (mainChatPage.StackView.view) {
                        mainChatPage.StackView.view.pop()
                    }
                }
            }
        }
    }

    background: Rectangle {
        color: "#f5f5f5"
    }

    // 主布局
    ColumnLayout {
        anchors.centerIn: parent
        spacing: 15
        width: parent.width * 0.9

        // 1. 好友列表展示区域
        Frame {
            Layout.fillWidth: true
            Layout.preferredHeight: 350 // 给列表更多空间
            background: Rectangle { color: "white"; border.color: "#ddd"; radius: 5 }

            ListView {
                id: friendListView
                anchors.fill: parent
                clip: true
                model: chatClient.friendList

                delegate: ItemDelegate {
                    width: parent.width
                    text: modelData
                    onClicked: {
                        // 点击后直接选中并准备聊天，或者直接跳转
                        targetUserField_hidden.text = modelData
                    }
                }
                ScrollIndicator.vertical: ScrollIndicator { }
            }
        }

        // 隐藏一个用来记录选中好友的 Text（或者直接用变量）
        Text { id: targetUserField_hidden; visible: false }

        // 2. 功能按钮区
        GridLayout {
            columns: 2
            Layout.fillWidth: true
            columnSpacing: 10

            // --- 跳转到添加页面 ---
            Button {
                text: "➕ 添加好友"
                Layout.fillWidth: true
                onClicked: {
                    // 跳转到新页面
                    mainChatPage.StackView.view.push("qrc:/AddFriendPage.qml")
                }
            }

            // --- 删除好友 ---
            Button {
                text: "🗑️ 删除好友"
                Layout.fillWidth: true
                contentItem: Text {
                    text: parent.text; color: "red"
                    horizontalAlignment: Text.AlignHCenter
                }
                onClicked: {
                    let name = targetUserField_hidden.text
                    if (name !== "") {
                        // 调用删除逻辑
                    } else {
                        statusLabel.text = "请先在列表中选择一个好友"
                    }
                }
            }

            // --- 进入聊天 (占据整行) ---
            Button {
                text: "💬 开始聊天"
                Layout.fillWidth: true
                Layout.columnSpan: 2
                highlighted: true
                onClicked: {
                    let name = targetUserField_hidden.text
                    if (name !== "") {
                        chatClient.setCurrentChatTarget(name);
                        mainChatPage.StackView.view.push("qrc:/chat.qml", { "chatTarget": name })
                    } else {
                        statusLabel.text = "请先点击列表选择好友"
                    }
                }
            }
        }

        Text {
            id: statusLabel
            text: targetUserField_hidden.text === "" ? "请选择一个好友开始操作" : "当前选中: " + targetUserField_hidden.text
            color: "#666"
            Layout.alignment: Qt.AlignHCenter
        }
    }
}