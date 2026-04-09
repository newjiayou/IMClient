import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: registerPage
    title: "用户注册"

    // 控制页面交互状态的属性
    property bool isRegistering: false

    // 【核心】监听 C++ ChatClient 传回的注册结果信号
    Connections {
        target: chatClient // 确保 main.cpp 中已将 chatClient 注册为上下文属性

        // 对应 C++: void registerResultReceived(bool success, const QString &message)
        function onRegisterResultReceived(success, message) {
            isRegistering = false // 收到回复，解除界面锁定

            statusLabel.text = message
            statusLabel.color = success ? "#07C160" : "red" // 成功绿色，失败红色

            if (success) {
                // 注册成功：清空输入并延迟 2 秒返回登录页
                regUserField.text = ""
                regPasswordField.text = ""
                regConfirmField.text = ""
                autoBackTimer.start()
            }
        }
    }

    // 顶部导航栏
    header: ToolBar {
        background: Rectangle { color: "#f8f8f8" }
        RowLayout {
            anchors.fill: parent
            ToolButton {
                text: "‹ 返回登录"
                enabled: !isRegistering
                onClicked: registerPage.StackView.view.pop()
            }
            Label {
                text: "新用户注册"
                elide: Label.ElideRight
                verticalAlignment: Qt.AlignVCenter
                horizontalAlignment: Qt.AlignHCenter
                Layout.fillWidth: true
                font.bold: true
                font.pixelSize: 18
            }
            Item { Layout.preferredWidth: 40 } // 占位，保持标题居中
        }
    }

    // 主背景
    background: Rectangle {
        color: "#ffffff"
    }

    // 页面内容布局
    ColumnLayout {
        anchors.centerIn: parent
        width: parent.width * 0.85
        spacing: 15

        // 图标或占位
        Text {
            text: "📝"
            font.pixelSize: 60
            Layout.alignment: Qt.AlignHCenter
        }

        Text {
            text: "创建新账号"
            font.pixelSize: 22
            font.bold: true
            Layout.alignment: Qt.AlignHCenter
            bottomPadding: 10
        }

        // 账号输入框
        TextField {
            id: regUserField
            placeholderText: "请输入用户名"
            Layout.fillWidth: true
            enabled: !isRegistering
            selectByMouse: true
            background: Rectangle {
                implicitHeight: 45
                border.color: regUserField.activeFocus ? "#07C160" : "#ddd"
                radius: 4
            }
        }

        // 密码输入框
        TextField {
            id: regPasswordField
            placeholderText: "设置密码"
            echoMode: TextInput.Password
            Layout.fillWidth: true
            enabled: !isRegistering
            selectByMouse: true
            background: Rectangle {
                implicitHeight: 45
                border.color: regPasswordField.activeFocus ? "#07C160" : "#ddd"
                radius: 4
            }
        }

        // 确认密码输入框
        TextField {
            id: regConfirmField
            placeholderText: "确认密码"
            echoMode: TextInput.Password
            Layout.fillWidth: true
            enabled: !isRegistering
            selectByMouse: true
            background: Rectangle {
                implicitHeight: 45
                border.color: regConfirmField.activeFocus ? "#07C160" : "#ddd"
                radius: 4
            }
        }

        // 状态/错误信息提示
        Text {
            id: statusLabel
            text: ""
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            font.pixelSize: 14
            visible: text !== ""
        }

        // 间距占位
        Item { Layout.preferredHeight: 10 }

        // 注册提交按钮
        Button {
            id: submitBtn
            text: isRegistering ? "提交中..." : "立即注册"
            Layout.fillWidth: true
            Layout.preferredHeight: 48
            enabled: !isRegistering && regUserField.text !== "" && regPasswordField.text !== ""

            contentItem: Text {
                text: submitBtn.text
                color: "white"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.bold: true
                font.pixelSize: 16
            }

            background: Rectangle {
                color: submitBtn.enabled ? (submitBtn.pressed ? "#06ad56" : "#07C160") : "#ccc"
                radius: 4
            }

            onClicked: {
                let u = regUserField.text.trim()
                let p = regPasswordField.text.trim()
                let cp = regConfirmField.text.trim()

                // 1. 本地逻辑校验
                if (u.length < 3) {
                    statusLabel.text = "用户名至少需要 3 个字符"
                    statusLabel.color = "red"
                    return
                }

                if (p !== cp) {
                    statusLabel.text = "两次输入的密码不一致"
                    statusLabel.color = "red"
                    return
                }

                // 2. 进入注册中状态，调用 C++ 接口
                statusLabel.text = "正在连接 Nginx 网关..."
                statusLabel.color = "blue"
                isRegistering = true

                // 调用 ChatClient 中你增加的 registerAccount 函数
                chatClient.registerAccount(u, p)
            }
        }

        // 辅助说明
        Text {
            text: "点击注册即表示同意用户协议"
            font.pixelSize: 12
            color: "#999"
            Layout.alignment: Qt.AlignHCenter
        }
    }

    // 成功注册后的自动返回定时器
    Timer {
        id: autoBackTimer
        interval: 2000
        onTriggered: {
            if (registerPage.StackView.view) {
                registerPage.StackView.view.pop()
            }
        }
    }
}