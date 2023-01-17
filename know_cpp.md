- 1.BaseConnection 与 TcpConnection
    - 子类和父类构造函数初始化
    - 父类只声明有参构造函数, 子类在初始化的时候需要显示调用父类带参构造函数
    ```
    TcpConnection::TcpConnection(boost::asio::ip::tcp::socket socket)
    : BaseConnection(std::move(socket)) {}
    ```