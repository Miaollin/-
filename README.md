# Low-Latency Trading System

一个使用 C++20 编写的低延迟交易系统学习项目。项目覆盖交易所与交易客户端两侧：交易所侧包含订单接入、FIFO 请求排序、撮合引擎、订单簿及行情发布；客户端侧正在实现订单网关、行情恢复、策略订单簿、持仓与风控组件。

> 项目仍处于学习和开发阶段，仅用于技术研究，不适合真实交易或生产环境。

## 系统架构

```text
Trading Client（开发中）
├── OrderManager / RiskManager / PositionKeeper
├── OrderGateway ───────── TCP 订单 ────────────┐
└── MarketDataConsumer ◄── 增量/快照 UDP 组播 ─┼──────────────┐
                                                ▼              │
                                         OrderServer           │
                                                │              │
                                         FIFOSequencer         │
                                                │              │
                                                ▼              │
                                        MatchingEngine         │
                                                │              │
                              ┌─────────────────┴──────────┐   │
                              ▼                            ▼   │
                    TCP 客户端响应             MarketDataPublisher
                                                           │
                                      ┌────────────────────┴──────────┐
                                      ▼                               ▼
                           增量行情 233.252.14.3:20001      SnapshotSynthesizer
                                                                      │
                                                           快照 233.252.14.1:20000
```

默认网络接口为回环接口 `lo`。快照合成器维护订单状态，并每 60 秒发布一次完整快照。

## 当前功能

交易所侧：

- 固定容量低延迟队列与预分配对象内存池
- 异步日志、非阻塞 TCP、epoll 和 UDP 组播封装
- 客户端请求/响应及客户端序列号校验
- 按接收时间排序的 FIFO Sequencer
- 每个交易品种独立的限价订单簿
- 新增、撤单和价格优先/时间优先撮合
- 成交、订单新增、修改和撤销的增量行情发布
- 基于增量行情维护状态并定时发布市场快照

交易客户端侧（开发中）：

- TCP 订单网关及请求/响应序列号管理
- 增量行情消费、丢包检测和快照同步逻辑
- 策略侧市场订单簿与 BBO 更新
- 特征计算、订单管理、持仓/PnL 跟踪和交易前风控框架
- `RANDOM`、`MAKER`、`TAKER` 算法类型与逐品种风控配置

## 项目结构

```text
.
├── CMakeLists.txt                  # 顶层构建配置
├── common/                        # 队列、内存池、日志、网络与公共类型
├── exchange/                      # 交易所侧
│   ├── exchange_main.cpp          # 交易所程序入口
│   ├── matcher/                   # 撮合引擎与订单簿
│   ├── order_server/              # 订单接入、校验与 FIFO 排序
│   └── market_data/               # 增量行情发布与快照合成
└── trading/                       # 交易客户端侧（开发中）
    ├── market_data/               # 行情消费、丢包检测与快照恢复
    ├── order_gw/                  # TCP 订单网关
    └── strategy/                  # 订单簿、特征、订单、持仓与风控
```

## 环境要求

- Linux（依赖 epoll、CPU affinity 等 Linux 接口）
- GCC（当前 CMake 配置使用 `g++`）
- 支持 C++20 的编译器与标准库
- CMake 3.0+
- Ninja
- POSIX Threads

Ubuntu/Debian 可安装：

```bash
sudo apt update
sudo apt install build-essential cmake ninja-build
```

## 编译

使用 CMake 和 Ninja 配置、编译：

```bash
cmake -S . -B cmake-build-release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build cmake-build-release -j 4
```

> **当前构建状态：** 顶层 `CMakeLists.txt` 已声明 `exchange_main` 和 `trading_main` 两个可执行目标，但 `trading/trading_main.cpp` 尚未加入仓库，因此全新执行 CMake 配置会报错。完成交易端入口后，预期产物为 `cmake-build-release/exchange_main` 和 `cmake-build-release/trading_main`。

## 运行交易所

在已有且有效的构建目录中运行：

```bash
./cmake-build-release/exchange_main
```

交易所进程会启动：

- `MatchingEngine`
- `OrderServer`
- `MarketDataPublisher`
- `SnapshotSynthesizer`

运行日志写入项目根目录下的 `exchange_*.log` 文件。按 `Ctrl+C` 退出。

交易客户端入口仍在开发中，目前没有可运行的 `trading_main`。

## 默认网络配置

| 服务 | 协议 | 地址/接口 | 端口 |
| --- | --- | --- | ---: |
| 订单网关 | TCP | `lo` | `12345` |
| 增量行情 | UDP 组播 | `233.252.14.3` | `20001` |
| 快照行情 | UDP 组播 | `233.252.14.1` | `20000` |

当前交易所配置直接定义在 `exchange/exchange_main.cpp` 中。

## 开发状态与限制

后续仍需重点完善：

- 完成 `trading_main`，串联行情消费者、订单网关和交易引擎
- 完善并验证交易端策略、持仓、PnL 与风控链路
- 优雅、安全的启动与关闭流程
- 网络消息的显式序列化、结构体布局和字节序处理
- 网络断连、非法客户端 ID、越界订单等异常输入防护
- 完整验证增量行情丢包检测与客户端侧快照恢复
- 线程生命周期、资源回收及错误处理
- 单元测试、集成测试、压力测试与延迟基准
- 可配置的接口、地址、端口和 CPU 亲和性

当前网络消息主要通过内存结构直接传输，并使用 `reinterpret_cast` 解析。跨平台或生产级通信需要稳定的线协议，不能依赖本机结构体布局。

## 学习重点

- C++20 类型系统、模板与 RAII
- 无锁/低锁数据结构和预分配对象池
- Linux 非阻塞网络编程、epoll 与 UDP 组播
- 价格优先、时间优先订单撮合
- 增量行情、市场快照与序列号恢复机制
- 策略订单管理、持仓核算与交易前风控
- 延迟测量、异步日志与线程 CPU 亲和性

## 声明

本项目仅用于学习和技术研究，不构成投资建议，也不保证满足生产环境所需的正确性、安全性或可靠性。
