import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: loginPage
    title: "登录"
    property bool isLoggingIn: false

    Connections {
        target: chatClient
        function onLoginResultReceived(success, message) {
            isLoggingIn = false
            if (success) {
                console.log("登录成功，正在跳转...")
                if (loginPage.StackView.view) {
                    loginPage.StackView.view.push("qrc:/desktop.qml")
                }
            } else {
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
        spacing: 15 // 稍微缩小间距，看起来更紧凑
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
            enabled: !isLoggingIn
        }

        TextField {
            id: passwordField
            placeholderText: "请输入密码"
            echoMode: TextInput.Password
            Layout.fillWidth: true
            enabled: !isLoggingIn
        }

        Text {
            id: errorLabel
            color: "red"
            text: ""
            visible: false
            Layout.alignment: Qt.AlignHCenter
        }

        // --- 登录按钮 ---
        Button {
            id: loginButton
            text: isLoggingIn ? "正在验证..." : "登录"
            enabled: !isLoggingIn
            Layout.fillWidth: true
            highlighted: true
            onClicked: {
                let uname = usernameField.text.trim()
                let pwd = passwordField.text.trim()
                if (uname === "" || pwd === "") {
                    errorLabel.text = "账号或密码不能为空"
                    errorLabel.visible = true
                    return
                }
                errorLabel.visible = false
                isLoggingIn = true
                chatClient.sendidentify(uname, pwd)
            }
        }

        // --- 【新增】注册按钮 ---
        Button {
            id: registerButton
            text: "没有账号？点击注册"
            Layout.fillWidth: true
            flat: true // 扁平化样式，看起来像个链接，不会太显眼
            enabled: !isLoggingIn // 登录过程中也不让点跳转

            // 设置文字颜色，使其看起来更像可点击的链接
            contentItem: Text {
                text: registerButton.text
                font.pixelSize: 14
                color: registerButton.down ? "#004080" : "#0066cc"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            onClicked: {
                console.log("前往注册页面...")
                if (loginPage.StackView.view) {
                    // 跳转到注册页面
                    loginPage.StackView.view.push("qrc:/regist.qml")
                }
            }
        }
    }
}