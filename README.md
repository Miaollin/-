# Low-Latency Trading System

一个使用 C++20 编写的低延迟交易系统学习项目。系统包含订单接入、FIFO 请求排序、撮合引擎、订单簿，以及增量行情与快照行情发布链路。

> 项目仍处于学习和开发阶段，仅用于技术研究，不适合真实交易或生产环境。

## 系统架构

```text
交易客户端
    │ TCP 订单请求（127.0.0.1:12345）
    ▼
OrderServer ── 序列号校验 ──► FIFOSequencer
    │                              │
    │ 客户端响应                   ▼
    ◄──────────────────── ClientRequestLFQueue
                                   │
                                   ▼
                            MatchingEngine
                                   │
                     ┌─────────────┴─────────────┐
                     ▼                           ▼
          ClientResponseLFQueue       MEMarketUpdateLFQueue
                                                 │
                                                 ▼
                                      MarketDataPublisher
                                         │             │
                          增量行情组播 ◄──┘             └──► SnapshotSynthesizer
                          233.252.14.3:20001                  │
                                                快照行情组播 ▼
                                                233.252.14.1:20000
```

默认网络接口为回环接口 `lo`。快照合成器维护订单状态，并每 60 秒发布一次完整快照。

## 当前功能

- 固定容量低延迟队列与预分配对象内存池
- 异步日志记录
- Linux 非阻塞 TCP、epoll 和 UDP 组播封装
- 客户端订单请求/响应及客户端序列号校验
- 按接收时间排序的 FIFO Sequencer
- 每个交易品种独立的限价订单簿
- 新增、撤单和价格优先/时间优先撮合
- 成交、订单新增、修改和撤销的增量行情发布
- 基于增量行情维护状态并定时发布市场快照
- CMake + Ninja Release 构建

## 项目结构

```text
.
├── CMakeLists.txt                  # 顶层构建配置与 exchange_main 目标
├── build.sh                       # Release 构建脚本
├── common/                        # 公共基础设施
│   ├── lf_queue.h                 # 固定容量队列
│   ├── logging.h                  # 异步日志
│   ├── mem_pool.h                 # 对象内存池
│   ├── mcast_socket.*             # UDP 组播 Socket
│   ├── tcp_server.*               # 基于 epoll 的 TCP 服务端
│   ├── tcp_socket.*               # TCP Socket 封装
│   ├── thread_utils.h             # 线程创建与 CPU 亲和性
│   └── types.h                    # 订单、价格、数量等公共类型
└── exchange/
    ├── exchange_main.cpp          # 交易所程序入口
    ├── matcher/                   # 撮合引擎与订单簿
    ├── order_server/              # 订单接入、校验与 FIFO 排序
    └── market_data/               # 增量行情发布与快照合成
```

## 环境要求

- Linux（依赖 epoll、CPU affinity 等 Linux 接口）
- GCC（当前 CMake 配置使用 `g++`）
- CMake 3.0+
- Ninja
- 支持 C++20 的标准库与编译器
- POSIX Threads

Ubuntu/Debian 可安装以下构建工具：

```bash
sudo apt update
sudo apt install build-essential cmake ninja-build
```

## 编译

在项目根目录执行：

```bash
chmod +x build.sh
./build.sh
```

脚本会生成 `cmake-build-release/`，执行一次 clean build，并产出：

```text
cmake-build-release/exchange_main
```

也可以手动执行 CMake：

```bash
cmake -S . -B cmake-build-release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build cmake-build-release -j 4
```

## 运行

```bash
./cmake-build-release/exchange_main
```

启动后会运行以下后台组件：

- `MatchingEngine`
- `OrderServer`
- `MarketDataPublisher`
- `SnapshotSynthesizer`

运行日志会写入项目根目录下的 `exchange_*.log` 文件。按 `Ctrl+C` 退出。

## 默认网络配置

| 服务 | 协议 | 地址/接口 | 端口 |
| --- | --- | --- | ---: |
| 订单网关 | TCP | `lo` | `12345` |
| 增量行情 | UDP 组播 | `233.252.14.3` | `20001` |
| 快照行情 | UDP 组播 | `233.252.14.1` | `20000` |

当前配置直接定义在 `exchange/exchange_main.cpp` 中。

## 开发状态与限制

后续仍需重点完善：

- 优雅、安全的启动与关闭流程
- 网络消息的显式序列化、结构体布局和字节序处理
- 网络断连、非法客户端 ID、越界订单等异常输入防护
- 增量行情丢包检测与客户端侧快照恢复
- 线程生命周期、资源回收及错误处理
- 单元测试、集成测试、压力测试与延迟基准
- 可配置的接口、地址、端口和 CPU 亲和性

当前网络消息主要通过内存结构直接传输，并使用 `reinterpret_cast` 解析。跨平台或生产级通信需要稳定的线协议，不能依赖本机结构体布局。

## 学习重点

- C++20 类型系统、模板与 RAII
- 无锁/低锁数据结构
- 预分配内存与对象池
- Linux 非阻塞网络编程、epoll 与 UDP 组播
- 价格优先、时间优先订单撮合
- 增量行情、市场快照与序列号恢复机制
- 延迟测量、异步日志与线程 CPU 亲和性

## 声明

本项目仅用于学习和技术研究，不构成投资建议，也不保证满足生产环境所需的正确性、安全性或可靠性。
