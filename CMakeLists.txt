cmake_minimum_required(VERSION 3.16)
project(QmlChatApp VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 查找 Qt6 组件
find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick Network Sql)

# 1. 添加可执行文件 (仅包含 main.cpp)
qt_add_executable(appQmlChatApp
    main.cpp
)

# 2. 定义 QML 模块
# 注意：ChatClient 和 chatmodel 必须放在这里，因为它们用了 QML_ELEMENT
qt_add_qml_module(appQmlChatApp
    URI ChatApp
    VERSION 1.0
    QML_FILES
        main.qml
        chat.qml
        regist.qml
    SOURCES
        ChatClient.h ChatClient.cpp
        chatmodel.h chatmodel.cpp
    RESOURCES
        logintest.jpg
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
