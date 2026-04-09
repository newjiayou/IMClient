cmake_minimum_required(VERSION 3.16)
project(QmlChatApp VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 查找 Qt6 组件
find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick Network Sql )

# 1. 先定义可执行程序 (只放 C++ 源文件)
qt_add_executable(appQmlChatApp
    main.cpp
    chatclient.h chatclient.cpp
    chatmodel.h chatmodel.cpp
    databasemanager.h databasemanager.cpp
    protocolhandler.h protocolhandler.cpp
    backendworker.h backendworker.cpp

)

# 2. 将资源直接附加到目标程序 (这是 Qt6 的正确语法，解决 // 报错的核心)
qt_add_resources(appQmlChatApp "app_resources"
    PREFIX "/"
    FILES
        Main.qml
        chat.qml
        login.qml
        logintest.jpg
        desktop.qml
        AddFriendPage.qml
        regist.qml
)

# 3. 链接 Qt 库
target_link_libraries(appQmlChatApp
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        Qt6::Network
        Qt6::Sql
)
