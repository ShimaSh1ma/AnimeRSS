#if defined(_WIN32)
#define NOMINMAX
#endif

#include "ClientSocket.h"

#include "HttpRequest.h"
#include "HttpResponseParser.h"
#include "MSocket.h"
#include "SocketConstant.h"
#include "UrlParser.h"

#include <array>
#include <condition_variable>
#include <cstring>
#include <functional>
#include <future>
#include <iostream>
#include <limits>
#include <mutex>
#include <queue>
#include <random>
#include <stop_token>
#include <vector>

#include <shared_mutex>

#include <iostream>

class JThreadPool {
  public:
    explicit JThreadPool(size_t threadCount = std::thread::hardware_concurrency()) : stopFlag(false) {
        for (size_t i = 0; i < threadCount; ++i) {
            workers.emplace_back([this](std::stop_token stoken) { workerLoop(stoken); });
        }
    }

    ~JThreadPool() {
        {
            std::unique_lock lock(queueMutex);
            stopFlag = true;
        }
        condition.notify_all();
    }

    template <typename Func, typename... Args> auto submit(Func&& func, Args&&... args) -> std::future<typename std::invoke_result_t<Func, Args...>> {
        using ReturnType = typename std::invoke_result_t<Func, Args...>;

        auto taskPtr = std::make_shared<std::packaged_task<ReturnType()>>(std::bind(std::forward<Func>(func), std::forward<Args>(args)...));

        std::future<ReturnType> resultFuture = taskPtr->get_future();

        {
            std::unique_lock lock(queueMutex);
            if (stopFlag)
                throw std::runtime_error("ThreadPool stopped, can't submit task");

            tasks.emplace([taskPtr]() {
                try {
                    (*taskPtr)();
                } catch (const std::exception& e) {
                    std::cerr << "Task exception: " << e.what() << std::endl;
                    throw;
                }
            });
        }

        condition.notify_one();
        return resultFuture;
    }

  private:
    void workerLoop(std::stop_token stoken) {
        while (!stoken.stop_requested()) {
            std::function<void()> task;

            {
                std::unique_lock lock(queueMutex);
                condition.wait(lock, [this, &stoken] { return stopFlag || !tasks.empty() || stoken.stop_requested(); });

                if ((stopFlag || stoken.stop_requested()) && tasks.empty())
                    return;

                task = std::move(tasks.front());
                tasks.pop();
            }

            try {
                task();
            } catch (...) {
            }
        }
    }

    std::vector<std::jthread> workers;
    std::queue<std::function<void()>> tasks;

    std::mutex queueMutex;
    std::condition_variable condition;
    bool stopFlag;
};

// 调用select函数传入的判断类型
enum class selectType
{
    READ,
    WRITE,
    READ_AND_WRITE
};

// 读写socket池并发控制锁
static std::shared_mutex socketPoolMutex;

constexpr const size_t timeWaitSeconds = 10;

// socket池
std::unordered_map<socketIndex, std::unique_ptr<MSocket>> ClientSocket::socketPool{};

// 设置socket非阻塞
bool setSocketNonblocked(const SOCKET socket);
// 判断socket连接状态
bool waitForConnection(const SOCKET socket, int timeout_sec);
// 使用select检查套接字是否可用
bool selectSocket(const SOCKET socket, selectType type, int timeoutSecond = timeWaitSeconds);

void WSAInit() {
#if defined(_WIN32)
    WSADATA wsaData;
    // 初始化socket环境
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        exit(1);
        return;
    }
#endif
    sslInit();
}

void WSAClean() {
#if defined(_WIN32)
    WSACleanup();
#endif
}

void sslInit() {
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
}

socketIndex ClientSocket::creatSocket(const std::string& _host, const std::string& _port) {
    // 生成随机数做索引
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<socketIndex> distribution(0, std::numeric_limits<socketIndex>::max());

    socketIndex index;
    bool isIndexUnique = false;
    std::unique_lock<std::shared_mutex> uniquelock(socketPoolMutex);
    do {
        index = distribution(mt);
        isIndexUnique = socketPool.find(index) == socketPool.end();
    } while (!isIndexUnique);
    // 添加到统一管理池
    socketPool.insert({index, std::make_unique<MSocket>(_host.c_str(), _port.c_str())});
    return index;
}

MSocket* ClientSocket::findSocket(socketIndex& index) {
    std::shared_lock<std::shared_mutex> sharedlock(socketPoolMutex);
    auto it = socketPool.find(index);
    if (it == socketPool.end()) {
        index = -1;
        return nullptr;
    }
    return it->second.get();
}

void ClientSocket::deleteSocket(socketIndex& index) {
    std::unique_lock<std::shared_mutex> uniquelock(socketPoolMutex);
    socketPool.erase(index);
    index = -1;
}

socketIndex ClientSocket::reuseSocket(const std::string& _host) {
    std::shared_lock<std::shared_mutex> sharedlock(socketPoolMutex);
    for (const auto& pair : socketPool) {
        if (pair.second->getHost() != _host) {
            continue;
        }
        bool expected = false;
        if (pair.second->isBusy.compare_exchange_weak(expected, true)) {
            return pair.first;
        }
    }
    return -1;
}

socketIndex ClientSocket::connectToServer(const std::string& _host, const std::string& _port) {
    socketIndex index = ClientSocket::reuseSocket(_host);
    if (index != -1 && selectSocket(findSocket(index)->socket, selectType::WRITE)) {
        return index;
    } else {
        deleteSocket(index);
    }

    MSocket* _temp;
    do {
        index = creatSocket(_host, _port);
        _temp = findSocket(index);
    } while (_temp == nullptr);
    MSocket& _socket = *_temp;

    memset(&(_socket.ipAddr), 0, sizeof(_socket.ipAddr));
    _socket.ipAddr.ai_family = AF_UNSPEC;
    _socket.ipAddr.ai_socktype = SOCK_STREAM;
    _socket.ipAddr.ai_protocol = IPPROTO_TCP;

    _socket.result = getaddrinfo(_socket.host.c_str(), _socket.port.c_str(), &_socket.ipAddr, &_socket.ipResult);
    if (_socket.result != 0) {
        _socket.errorLog = _M_DNS_ERR + std::to_string(WSAGetLastError()) + "\n";
        deleteSocket(index);
        return index;
    }

    _socket.socket = socket(_socket.ipResult->ai_family, _socket.ipResult->ai_socktype, _socket.ipResult->ai_protocol);
    if (_socket.socket == INVALID_SOCKET || !setSocketNonblocked(_socket.socket)) {
        _socket.errorLog = _M_SOCKET_CREATE_ERR + std::to_string(WSAGetLastError()) + "\n";
        deleteSocket(index);
        return index;
    }

    _socket.result = connect(_socket.socket, _socket.ipResult->ai_addr, static_cast<int>(_socket.ipResult->ai_addrlen));
    bool errorOccurred;
#if defined(_WIN32)
    errorOccurred = _socket.result == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK;
#else
    errorOccurred = _socket.result == SOCKET_ERROR && errno != EINPROGRESS;
#endif
    if (errorOccurred) {
        _socket.errorLog = _M_SOCKET_CONNECT_ERR + std::to_string(WSAGetLastError());
        deleteSocket(index);
        return index;
    }

    freeaddrinfo(_socket.ipResult);

    if (!waitForConnection(_socket.socket, timeWaitSeconds)) {
        _socket.errorLog = _M_SOCKET_CONNECT_ERR + std::to_string(WSAGetLastError());
        deleteSocket(index);
        return index;
    }

    return index;
}

socketIndex ClientSocket::sslConnect(socketIndex& index, const std::string& targetHost, const std::string& targetPort) {
    MSocket* _temp = findSocket(index);
    if (_temp == nullptr) {
        return false;
    }
    MSocket& _socket = *_temp;

    // 建立SSL上下文
    _socket.ctx = SSL_CTX_new(_socket.meth);
    if (_socket.ctx == NULL) {
        _socket.errorLog = _M_SSL_CONTEXT_ERR;
        deleteSocket(index);
        return index;
    }

    // 建立ssl连接
    _socket.sslSocket = SSL_new(_socket.ctx);
    if (_socket.sslSocket == NULL) {
        _socket.errorLog = _M_SSL_CREATE_ERR;
        deleteSocket(index);
        return index;
    }

    SSL_set_mode(_socket.sslSocket, SSL_MODE_AUTO_RETRY);            // 自动重试
    SSL_set_tlsext_host_name(_socket.sslSocket, targetHost.c_str()); // tls握手失败时尝试指定主机名

    // 绑定套接字
    SSL_set_fd(_socket.sslSocket, static_cast<int>(_socket.socket));

    // 建立SSL连接
    while (true) {
        _socket.result = SSL_connect(_socket.sslSocket);
        if (_socket.result == 1) {
            // SSL连接成功
            return index;
        }

        // 获取错误码
        _socket.result = SSL_get_error(_socket.sslSocket, _socket.result);

        if (_socket.result == SSL_ERROR_WANT_READ || _socket.result == SSL_ERROR_WANT_WRITE) {
            if (_socket.result == SSL_ERROR_WANT_READ) {
                if (selectSocket(_socket.socket, selectType::READ)) {
                    continue;
                }
            } else if (_socket.result == SSL_ERROR_WANT_WRITE) {
                if (selectSocket(_socket.socket, selectType::WRITE)) {
                    continue;
                }
            }
            _socket.errorLog = _M_SSL_CONNECT_ERR;
            deleteSocket(index);
            return index;
        }
        _socket.errorLog = _M_SSL_CONNECT_ERR + std::to_string(_socket.result);
        deleteSocket(index);
        return index;
    }
}

socketIndex ClientSocket::connectToServerSSL(const std::string& _host, const std::string& _port) {
    socketIndex index = ClientSocket::reuseSocket(_host);
    if (index != -1 && selectSocket(findSocket(index)->socket, selectType::WRITE)) {
        return index;
    } else {
        deleteSocket(index);
    }

    MSocket* _temp;
    do {
        index = creatSocket(_host, _port);
        _temp = findSocket(index);
    } while (_temp == nullptr);
    MSocket& _socket = *_temp;

    // 初始化socket信息结构体
    memset(&(_socket.ipAddr), 0, sizeof(_socket.ipAddr)); // 置零避免错误
    _socket.ipAddr.ai_family = AF_UNSPEC;                 // 不指定ip协议簇
    _socket.ipAddr.ai_socktype = SOCK_STREAM;             // 选择TCP
    _socket.ipAddr.ai_protocol = IPPROTO_TCP;             // 选择TCP

    // 通过getaddrinfo函数进行DNS请求
    _socket.result = getaddrinfo(_socket.host.c_str(), _socket.port.c_str(), &_socket.ipAddr, &_socket.ipResult);
    if (_socket.result != 0) {
        _socket.errorLog = _M_DNS_ERR + std::to_string(WSAGetLastError()) + "\n";
        freeaddrinfo(_socket.ipResult);
        deleteSocket(index);
        return index;
    }

    _socket.socket = socket(_socket.ipResult->ai_family, _socket.ipResult->ai_socktype, _socket.ipResult->ai_protocol);
    // 调用套接字函数为socket对象添加参数
    if (_socket.socket == INVALID_SOCKET || !setSocketNonblocked(_socket.socket)) {
        _socket.errorLog = _M_SOCKET_CREATE_ERR + std::to_string(WSAGetLastError()) + "\n";
        freeaddrinfo(_socket.ipResult);
        deleteSocket(index);
        return index;
    }

    // 连接到远程服务器
    _socket.result = connect(_socket.socket, _socket.ipResult->ai_addr, static_cast<int>(_socket.ipResult->ai_addrlen));
    bool errorOccurred;
#if defined(_WIN32)
    errorOccurred = _socket.result == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK;
#else
    errorOccurred = _socket.result == SOCKET_ERROR && errno == EINPROGRESS;
#endif
    if (errorOccurred) {
        _socket.errorLog = _M_SOCKET_CONNECT_ERR + std::to_string(WSAGetLastError());
        freeaddrinfo(_socket.ipResult);
        deleteSocket(index);
        return index;
    }

    freeaddrinfo(_socket.ipResult);

    if (!waitForConnection(_socket.socket, timeWaitSeconds)) {
        _socket.errorLog = _M_SOCKET_CONNECT_ERR + std::to_string(WSAGetLastError());
        deleteSocket(index);
        return index;
    }

    // 建立SSL连接
    return ClientSocket::sslConnect(index, _host, _port);
}

bool ClientSocket::socketSend(socketIndex& index, const std::string& msg) {
    MSocket* _temp = findSocket(index);
    if (_temp == nullptr) {
        return false;
    }
    MSocket& _socket = *_temp;

    size_t sentLength = 0;
    while (sentLength < msg.length()) {
        _socket.result = send(_socket.socket, msg.c_str() + sentLength, static_cast<int>(msg.length() - sentLength), 0);
        if (_socket.result == SOCKET_ERROR) {
            if (WSAGetLastError() == WSAEWOULDBLOCK && selectSocket(_socket.socket, selectType::WRITE)) {
                continue;
            }
            _socket.errorLog = _M_SOCKET_SEND_ERR + std::to_string(WSAGetLastError());
            deleteSocket(index);
            return false;
        } else {
            sentLength += _socket.result;
        }
    }
    return true;
}

bool ClientSocket::socketSendSSL(socketIndex& index, const std::string& msg) {
    MSocket* _temp = findSocket(index);
    if (_temp == nullptr) {
        return false;
    }
    MSocket& _socket = *_temp;

    size_t sentLength = 0;
    while (sentLength < msg.length()) {
        _socket.result = SSL_write(_socket.sslSocket, msg.c_str() + sentLength, static_cast<int>(msg.length() - sentLength));
        if (_socket.result <= 0) {
            _socket.result = SSL_get_error(_socket.sslSocket, _socket.result);
            if (_socket.result == SSL_ERROR_WANT_WRITE && selectSocket(_socket.socket, selectType::WRITE)) {
                continue;
            }
            _socket.errorLog = _M_SSL_WRITE_ERR + _socket.result;
            deleteSocket(index);
            return false;
        } else {
            sentLength += _socket.result;
        }
    }
    return true;
}

std::unique_ptr<HttpResponseParser> ClientSocket::socketReceive(socketIndex& index) {
    MSocket* _temp = findSocket(index);
    if (_temp == nullptr) {
        return nullptr;
    }
    MSocket& _socket = *_temp;

    std::string receivedData;
    std::array<char, _BUFFER_SIZE> recvBuffer;
    size_t dividePos = std::string::npos;

    while (dividePos == std::string::npos) {
        _socket.result = recv(_socket.socket, recvBuffer.data(), _BUFFER_SIZE, 0);
        if (_socket.result <= 0) {
            if (WSAGetLastError() == WSAEWOULDBLOCK && selectSocket(_socket.socket, selectType::READ)) {
                continue;
            }
            _socket.errorLog = _M_SOCKET_RECV_ERR + std::to_string(WSAGetLastError());
            deleteSocket(index);
            return nullptr;
        }
        receivedData.append(recvBuffer.data(), _socket.result);
        dividePos = receivedData.find("\r\n\r\n");
    }

    std::string headData = receivedData.substr(0, dividePos);
    dividePos += 4;
    std::string bodyData = receivedData.substr(dividePos);

    std::unique_ptr<HttpResponseParser> parser(new HttpResponseParser);
    parser->parseResponse(headData);

    if (parser->getHttpHead("Transfer-Encoding") == "chunked") {
        parser->recvByChunckedData(bodyData);
        while (true) {
            _socket.result = recv(_socket.socket, recvBuffer.data(), _BUFFER_SIZE, 0);
            if (_socket.result <= 0) {
                if (WSAGetLastError() == WSAEWOULDBLOCK && selectSocket(_socket.socket, selectType::READ)) {
                    continue;
                }
                _socket.errorLog = _M_SOCKET_RECV_ERR + std::to_string(WSAGetLastError());
                deleteSocket(index);
                return nullptr;
            }
            bodyData.append(recvBuffer.data(), _socket.result);
            if (parser->recvByChunckedData(bodyData) == 0) {
                break;
            }
        }
    }

    if (!parser->getHttpHead("Content-Length").empty()) {
        size_t restLength = parser->recvByContentLength(bodyData);
        while (restLength > 0) {
            _socket.result = recv(_socket.socket, recvBuffer.data(), static_cast<int>(std::min(_BUFFER_SIZE, restLength)), 0);
            if (_socket.result <= 0) {
                if (WSAGetLastError() == WSAEWOULDBLOCK && selectSocket(_socket.socket, selectType::READ)) {
                    continue;
                }
                _socket.errorLog = _M_SOCKET_RECV_ERR + std::to_string(WSAGetLastError());
                deleteSocket(index);
                return nullptr;
            }
            bodyData.append(recvBuffer.data(), _socket.result);
            restLength = parser->recvByContentLength(bodyData);
        }
    }

    return parser;
}

std::unique_ptr<HttpResponseParser> ClientSocket::socketReceiveSSL(socketIndex& index) {
    MSocket* _temp = findSocket(index);
    if (_temp == nullptr) {
        return nullptr;
    }
    MSocket& _socket = *_temp;

    std::string receivedData;
    std::array<char, _BUFFER_SIZE> recvBuffer;

    // 报文头与报文载荷分割点
    size_t dividePos = std::string::npos;

    while (dividePos == std::string::npos) {
        _socket.result = SSL_read(_socket.sslSocket, recvBuffer.data(), _BUFFER_SIZE);
        if (_socket.result <= 0) {
            _socket.result = SSL_get_error(_socket.sslSocket, _socket.result);
            if (_socket.result == SSL_ERROR_WANT_READ && selectSocket(_socket.socket, selectType::READ)) {
                continue;
            }
            _socket.errorLog = _M_SSL_WRITE_ERR + _socket.result;
            deleteSocket(index);
            return nullptr;
        }
        receivedData.append(recvBuffer.data(), _socket.result);
        dividePos = receivedData.find("\r\n\r\n");
    }

    // 获取http响应报头
    std::string headData = receivedData.substr(0, dividePos);
    dividePos += 4;
    // 获取已经读取到的报文内容
    std::string bodyData = receivedData.substr(dividePos);

    std::unique_ptr<HttpResponseParser> parser(new HttpResponseParser);
    // 解析http响应头
    parser->parseResponse(headData);

    if (parser->getHttpHead("Transfer-Encoding") == "chunked") {
        parser->recvByChunckedData(bodyData);
        while (true) {
            _socket.result = SSL_read(_socket.sslSocket, recvBuffer.data(), _BUFFER_SIZE);
            if (_socket.result <= 0) {
                _socket.result = SSL_get_error(_socket.sslSocket, _socket.result);
                if (_socket.result == SSL_ERROR_WANT_READ && selectSocket(_socket.socket, selectType::READ)) {
                    continue;
                }
                _socket.errorLog = _M_SSL_WRITE_ERR + _socket.result;
                deleteSocket(index);
                return nullptr;
            }
            bodyData.append(recvBuffer.data(), _socket.result);
            if (parser->recvByChunckedData(bodyData) == 0) {
                break;
            }
        }
    }

    if (!parser->getHttpHead("Content-Length").empty()) {
        size_t restLength = parser->recvByContentLength(bodyData);
        while (restLength > 0) {
            _socket.result = SSL_read(_socket.sslSocket, recvBuffer.data(), static_cast<int>(std::min(_BUFFER_SIZE, restLength)));
            if (_socket.result <= 0) {
                _socket.result = SSL_get_error(_socket.sslSocket, _socket.result);
                if (_socket.result == SSL_ERROR_WANT_READ && selectSocket(_socket.socket, selectType::READ, 30)) {
                    continue;
                }
                _socket.errorLog = _M_SSL_WRITE_ERR + _socket.result;
                deleteSocket(index);
                return nullptr;
            }
            bodyData.append(recvBuffer.data(), _socket.result);
            restLength = parser->recvByContentLength(bodyData);
        }
    }

    return parser;
}

void ClientSocket::releaseSocket(socketIndex& index) {
    MSocket* _temp = findSocket(index);
    if (_temp == nullptr) {
        return;
    }
    MSocket& _socket = *_temp;
    _socket.isBusy.store(false);
}

void ClientSocket::disconnectToServer(socketIndex& index) {
    deleteSocket(index);
}

bool setSocketNonblocked(const SOCKET socket) {
#if defined(_WIN32)
    unsigned long mode = 1;
    if (ioctlsocket(socket, FIONBIO, &mode) != 0) {
        return false;
    }
#endif

#if defined(__LINUX__) || defined(__APPLE__)
    int flags = fcntl(socket, F_GETFL, 0);
    if (flags < 0) {
        return false;
    }
    flags |= O_NONBLOCK;
    if (fcntl(socket, F_SETFL, flags) < 0) {
        return false;
    }
#endif
    return true;
}

bool waitForConnection(const SOCKET socket, int timeout_sec) {
    fd_set writefds;
    FD_ZERO(&writefds);
    FD_SET(socket, &writefds);

    struct timeval timeout;
    memset(&timeout, 0, sizeof(timeval));
    timeout.tv_sec = timeout_sec;

    if (!selectSocket(socket, selectType::WRITE)) {
        return false;
    }

    int optval;
    socklen_t optlen = sizeof(optval);
    if (getsockopt(socket, SOL_SOCKET, SO_ERROR, (char*)&optval, &optlen) < 0) {
        return false;
    }

    if (optval != 0) {
        return false;
    }

    return true;
}

bool selectSocket(const SOCKET socket, selectType type, int timeoutSecond) {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(socket, &fds);

    struct timeval timeout;
    memset(&timeout, 0, sizeof(timeval));
    timeout.tv_sec = timeoutSecond;

    int result = -1;
    if (type == selectType::READ) {
        result = select(static_cast<int>(socket + 1), &fds, NULL, NULL, &timeout);
    } else if (type == selectType::WRITE) {
        result = select(static_cast<int>(socket + 1), NULL, &fds, NULL, &timeout);
    } else if (type == selectType::READ_AND_WRITE) {
        result = select(static_cast<int>(socket + 1), &fds, &fds, NULL, &timeout);
    }

    if (result <= 0) {
        return false;
    }
    if (!FD_ISSET(socket, &fds)) {
        return false;
    }
    return true;
}

// 1. 发送 CONNECT 请求建立隧道（返回是否成功）
bool ClientSocket::httpProxyConnect(socketIndex& sIndex, const std::string& targetHost, const std::string& targetPort) {
    std::string connectRequest = "CONNECT " + targetHost + ":" + targetPort +
                                 " HTTP/1.1\r\n"
                                 "Host: " +
                                 targetHost + ":" + targetPort +
                                 "\r\n"
                                 "Proxy-Connection: Keep-Alive\r\n\r\n";

    if (!socketSend(sIndex, connectRequest)) {
        sIndex = -1;
        return false;
    }

    auto resp = socketReceive(sIndex);
    if (!resp) {
        sIndex = -1;
        return false;
    }

    if (resp->getStatusCode() == "200") {
        return true;
    }

    sIndex = -1;
    return false;
}

void ClientSocket::asyncRequestProxy(const std::string& proxyHost, const std::string& proxyPort, const std::string& targetHost, const std::string& targetPort,
                                     std::string httpsRequest, CallBack callBack) {
    static JThreadPool threadPool(3);
    threadPool.submit([proxyHost, proxyPort, targetHost, targetPort, request = std::move(httpsRequest), cb = std::move(callBack)]() {
        constexpr int maxRetries = 3;
        for (int attempt = 0; attempt < maxRetries; ++attempt) {
            socketIndex sIndex = ClientSocket::connectToServer(proxyHost, proxyPort);
            if (sIndex == -1) {
                continue;
            }

            if (!httpProxyConnect(sIndex, targetHost, targetPort)) {
                ClientSocket::disconnectToServer(sIndex);
                continue;
            }

            sslConnect(sIndex, targetHost, targetPort);
            if (sIndex == -1) {
                ClientSocket::disconnectToServer(sIndex);
                continue;
            }

            if (!ClientSocket::socketSendSSL(sIndex, request)) {
                ClientSocket::disconnectToServer(sIndex);
                continue;
            }

            auto resp = ClientSocket::socketReceiveSSL(sIndex);

            if (!resp) {
                continue;
            }

            if (resp->getStatusCode() != "200") {
                continue;
            }

            ClientSocket::releaseSocket(sIndex);
            if (cb) {
                cb(resp->getPayload());
            }
            return;
        }
    });
}

void ClientSocket::asyncRequest(const std::string& host, const std::string& port, std::string request, CallBack callBack) {
    static JThreadPool threadPool(3);
    threadPool.submit([=]() {
        std::string respCode;
        socketIndex sIndex = -1;
        int retryTimes = 3;
        while (respCode != "200" && retryTimes > 0) {
            --retryTimes;
            if (sIndex == -1) {
                sIndex = ClientSocket::connectToServerSSL(host, port);
                if (sIndex != -1) {
                    if (ClientSocket::socketSendSSL(sIndex, request)) {
                        std::unique_ptr<HttpResponseParser> resp = ClientSocket::socketReceiveSSL(sIndex);
                        if (resp == nullptr) {
                            continue;
                        }
                        respCode = resp->getStatusCode();
                        if (respCode == "200") {
                            if (callBack) {
                                callBack(resp->getPayload());
                            }
                        }
                    }
                }
            }
            ClientSocket::releaseSocket(sIndex);
        }
    });
}
