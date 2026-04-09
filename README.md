# QmlChatApp - 高可靠通讯客户端

基于 **Qt 6 (QML & C++)** 开发的即时通讯软件，采用逻辑与表现层完全解耦的架构，实现了自定义二进制协议通信、高可靠连接管理及本地数据持久化。

## 🛠 技术栈
- **前端**：QML, Qt Quick Controls 2
- **后端**：C++ 17, Qt Network (QTcpSocket), Qt SQL (SQLite)
- **构建工具**：CMake


## 📥 编译与运行
1. 环境要求：Qt 6.2+ (编译器建议 MinGW 或 MSVC)
2. 使用 Qt Creator 打开 `CMakeLists.txt`。
3. 编译并运行。默认连接本地服务器 `127.0.0.1:12345`。
