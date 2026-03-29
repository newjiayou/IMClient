import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: addFriendPage
    title: "添加好友"

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            ToolButton {
                text: "‹ 返回"
                onClicked: addFriendPage.StackView.view.pop()
            }
            Label {
                text: "添加好友"
                elide: Label.ElideRight
                verticalAlignment: Qt.AlignVCenter
                horizontalAlignment: Qt.AlignHCenter
                Layout.fillWidth: true
                font.bold: true
            }
            Item { Layout.preferredWidth: 40 } // 占位保持居中
        }
    }

    ColumnLayout {
        anchors.top: parent.top
        anchors.topMargin: 50
        anchors.horizontalCenter: parent.horizontalCenter
        width: parent.width * 0.8
        spacing: 20

        Text {
            text: "通过账号查找好友"
            font.pixelSize: 18
            color: "#333"
        }

        TextField {
            id: searchField
            placeholderText: "请输入对方账号..."
            Layout.fillWidth: true
            focus: true
            font.pixelSize: 16
        }

        Button {
            id: submitBtn
            text: "发送好友申请"
            Layout.fillWidth: true
            Layout.preferredHeight: 45
            highlighted: true
            enabled: searchField.text.trim() !== ""

            onClicked: {
                statusText.text = "正在发送请求..."
                statusText.color = "blue"
                // 调用 C++ 接口
                chatClient.addFriend(searchField.text.trim())
            }
        }

        Text {
            id: statusText
            text: ""
            Layout.alignment: Qt.AlignHCenter
            font.pixelSize: 14
        }
    }

    // 在这里监听添加结果，因为用户现在在这个页面等待反馈
    Connections {
        target: chatClient
        function onFriendActionResults(success, message) {
            statusText.text = message
            statusText.color = success ? "green" : "red"
            if (success) {
                searchField.text = "" // 成功后清空输入框
                // 可选：延迟 1 秒后自动返回
                // timer.start()
            }
        }
    }
}