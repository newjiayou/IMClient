import QtQuick
import QtQuick.Controls
ApplicationWindow
{
    width: 400
    height: 600
    visible: true
    title:"QQ模拟器"
    // ✅ 关键1：去除系统自带的方形边框和标题栏

    StackView{
        id:stackView
        anchors.fill:parent
        initialItem: "login.qml"
    }
}
