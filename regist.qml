import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
// 去掉版本号，Qt 6 默认加载最新版

Page {
    id: loginPage
    title: "登录"

    background: Rectangle {
        color: "#f0f0f0"
    }
    ColumnLayout {
        anchors.centerIn: parent
        spacing: 20
        width: parent.width * 0.8
        Image{
            source:"logintest.jpg"
            Layout.preferredWidth: 120//保持比例
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
        }

        TextField {
            id: passwordField
            placeholderText: "请输入密码"
            echoMode: TextInput.Password // 隐藏密码文字
            Layout.fillWidth: true
        }

        Button {
            text: "登录"
            Layout.fillWidth: true
            highlighted: true // 按钮高亮样式

            onClicked: {
                // 简单的逻辑判断
                if(usernameField.text === "") {
                    console.log("请输入用户名")
                    return
                }
                chatClient.currentUser =usernameField.text
                // --- 核心跳转代码 ---
                // push(页面文件路径, {属性传参})
                // StackView.view 获取当前页面所在的 StackView
                loginPage.StackView.view.push("chat.qml")
            }
        }
    }
}
