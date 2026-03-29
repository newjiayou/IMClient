import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: loginPage
    title: "登录"
    // 【新增 1】定义一个内部状态变量，用来控制按钮状态和文字显示
    property bool isLoggingIn: false

    // 【新增 2】监听来自 C++ ChatClient 的信号
    Connections {
        target: chatClient // 确保你的 main.cpp 中将 ChatClient 实例设为 qmlContext 的属性

        // 对应 C++ 中声明的信号 void loginResultReceived(bool success, const QString &message)
        function onLoginResultReceived(success, message) {
            // 只要收到回复（无论成败），就停止“登录中”状态
            isLoggingIn = false

            if (success) {
                console.log("登录成功，正在跳转...")
                // 【核心修改】只有在这里，收到服务端确认后，才允许跳转
                if (loginPage.StackView.view) {
                    // loginPage.StackView.view.push("qrc:/chat.qml")
                    loginPage.StackView.view.push("qrc:/desktop.qml")
                }
            } else {
                // 登录失败，显示错误信息
                errorLabel.text = message
                errorLabel.visible = true
            }
        }
    }

    background: Rectangle {
        color: "#f0f0f0"
    }

    ColumnLayout {
        anchors.centerIn: parent
        spacing: 20
        width: parent.width * 0.8

        Image {
            source: "logintest.jpg"
            Layout.preferredWidth: 120
            Layout.preferredHeight: 120
            fillMode: Image.PreserveAspectFit
            Layout.alignment: Qt.AlignHCenter
        }

        Text {
            text: "欢迎登录"
            font.pixelSize: 24
            font.bold: true
            Layout.alignment: Qt.AlignHCenter
        }

        TextField {
            id: usernameField
            placeholderText: "请输入账号"
            Layout.fillWidth: true
            // 登录中时禁止输入
            enabled: !isLoggingIn
        }

        TextField {
            id: passwordField
            placeholderText: "请输入密码"
            echoMode: TextInput.Password
            Layout.fillWidth: true
            // 登录中时禁止输入
            enabled: !isLoggingIn
        }

        // 【新增 3】错误提示文本（默认隐藏）
        Text {
            id: errorLabel
            color: "red"
            text: ""
            visible: false
            Layout.alignment: Qt.AlignHCenter
        }

        Button {
            id: loginButton
            // 【新增 4】动态切换文字和禁用状态
            text: isLoggingIn ? "正在验证..." : "登录"
            enabled: !isLoggingIn // 登录中禁止重复点击

            Layout.fillWidth: true
            highlighted: true

            onClicked: {
                // 1. 基础校验
                let uname = usernameField.text.trim()
                let pwd = passwordField.text.trim()

                if (uname === "" || pwd === "") {
                    errorLabel.text = "账号或密码不能为空"
                    errorLabel.visible = true
                    return
                }

                // 2. 隐藏之前的错误，进入“登录中”状态
                errorLabel.visible = false
                isLoggingIn = true

                // 3. 【核心修改】只负责发消息，不负责跳转
                // 发送 Type=4 的 JSON 包给服务器
                chatClient.sendidentify(uname, pwd)

                // 注意：这里删除了 chatClient.currentUser = uname
                // 建议在 C++ 端确认 success 后再设置 currentUser
            }
        }
    }
}