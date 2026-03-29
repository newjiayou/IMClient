#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "ChatClient.h"
#include <QDateTime>
#include "chatmodel.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setOrganizationName("MyChatApp");
    app.setApplicationName("QmlChatClient");
    ChatClient chatClient;
    if(chatClient.currentUser()=="")
    {
        chatClient.setCurrentUser("User_" + QString::number(QDateTime::currentMSecsSinceEpoch() % 1000));
    }
    chatClient.loadConfigAndConnect();
    QQmlApplicationEngine engine;

    // 【修改 1】：暴露 chatClient 给 QML
    engine.rootContext()->setContextProperty("chatClient", &chatClient);

    // 【修改 2】：不要在这里 new chatmodel，直接从 chatClient 获取，保证全局只有1个 Model
    engine.rootContext()->setContextProperty("chatModel", chatClient.chatModel());

    // [FIXED] 修正了 QML 文件的资源路径
    const QUrl url(u"qrc:/Main.qml"_qs);

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
                         if (!obj && url == objUrl)
                             QCoreApplication::exit(-1);
                     }, Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
