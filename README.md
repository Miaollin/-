# Low-Latency Trading System

一个使用 C++17 编写的低延迟交易系统学习项目。项目目标是从基础组件开始，逐步实现订单网关、FIFO 请求排序、撮合引擎、订单簿以及市场数据发布流程。

> 项目目前仍处于学习和开发阶段，不适合用于真实交易或生产环境。

## 项目结构

```text
.
├── common/                         # 公共基础设施
│   ├── lf_queue.h                  # 固定容量队列
│   ├── logging.h                   # 异步日志
│   ├── mem_pool.h                  # 对象内存池
│   ├── socket_utils.h              # Socket 工具函数
│   ├── tcp_server.*                # 基于 epoll 的 TCP 服务端
│   ├── tcp_socket.*                # TCP Socket 封装
│   ├── mcast_socket.*              # UDP 组播 Socket 封装
│   ├── thread_utils.h              # 线程创建及 CPU 亲和性
│   ├── time_utils.h                # 时间工具
│   └── types.h                     # 订单、价格、数量等公共类型
├── exchange/
│   ├── exchange_main.cpp           # 交易所程序入口
│   ├── matcher/                    # 撮合引擎与订单簿
│   ├── order_server/               # 订单接入、序列校验和 FIFO 排序
│   └── market_data/                # 市场数据消息与发布器
└── .vscode/                        # VS Code C++17 配置
```

## 当前功能

- 固定容量队列和对象内存池
- 异步日志记录
- 非阻塞 TCP、epoll 和 UDP 组播基础封装
- 客户端订单请求与响应消息
- 按接收时间排序的 FIFO Sequencer
- 每个交易品种独立的订单簿
- 限价单新增、撤单及基础撮合流程
- 成交、订单新增、修改和撤销市场数据消息

## 数据流

```text
客户端
  │ TCP 订单请求
  ▼
OrderServer
  │ 序列号校验
  ▼
FIFOSequencer
  │ 按接收时间排序
  ▼
ClientRequestLFQueue
  ▼
MatchingEngine ──► MEOrderBook
  │                    │
  │ 客户端响应          │ 市场数据更新
  ▼                    ▼
ClientResponseLFQueue  MEMarketUpdateLFQueue
```

## 环境要求

- Linux（使用了 epoll、CPU affinity 等 Linux 接口）
- Clang 或 GCC
- C++17
- POSIX Threads

推荐使用 VS Code，并安装 Microsoft C/C++ 或 clangd 扩展。

## 编译

项目暂时没有 CMake 或 Makefile，可以在项目根目录手动编译：

```bash
clang++ -std=c++17 -O2 -pthread -I. \
  exchange/exchange_main.cpp \
  exchange/matcher/matching_engine.cpp \
  exchange/matcher/me_order.cpp \
  exchange/matcher/me_order_book.cpp \
  exchange/order_server/order_server.cpp \
  exchange/market_data/market_data_publisher.cpp \
  common/tcp_server.cpp \
  common/tcp_socket.cpp \
  common/mcast_socket.cpp \
  -o exchange_app
```

也可以将 `clang++` 替换为 `g++`。

只进行语法检查：

```bash
find . -type f -name '*.cpp' -print0 | \
  xargs -0 clang++ -std=c++17 -Wall -Wextra -Wpedantic -I. -fsyntax-only
```

## 运行

```bash
./exchange_app
```

程序会启动撮合引擎并写入日志文件。按 `Ctrl+C` 退出。

## 开发状态与限制

项目仍在持续开发，目前需要继续完善：

- 订单服务器与主程序的完整启动和关闭流程
- 网络消息的序列化、字节序和安全解析
- FIFO Sequencer 与响应发送流程
- 市场数据增量发布和快照恢复
- 订单簿边界检查及异常输入处理
- 单元测试、集成测试和性能基准
- CMake 或 Makefile 构建配置

当前代码使用 `reinterpret_cast` 读取网络消息，实际跨平台通信时还需要处理结构体布局、内存对齐和网络字节序。

## 学习重点

这个项目适合用于练习：

- C++17 类型系统、模板和 RAII
- 无锁/低锁数据结构
- 预分配内存与对象池
- Linux 非阻塞网络编程和 epoll
- 订单簿及价格优先、时间优先撮合
- 延迟测量、日志和线程 CPU 亲和性

## 声明

本项目仅用于学习和技术研究，不构成投资建议，也不保证满足生产环境所需的正确性、安全性或可靠性。
