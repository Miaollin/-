# Low-Latency Trading System

一个使用 C++20 编写的低延迟交易系统学习项目。项目覆盖交易所与交易客户端两侧：交易所侧包含订单接入、FIFO 请求排序、撮合引擎、订单簿及行情发布；客户端侧包含订单网关、行情恢复、策略订单簿、特征计算、交易策略、持仓与风控组件。

> 项目仍处于学习和开发阶段，仅用于技术研究，不适合真实交易或生产环境。

## 系统架构

```text
Trading Client
├── TradeEngine
├── FeatureEngine / MarketMaker / LiquidityTaker
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

交易客户端侧：

- TCP 订单网关及请求/响应序列号管理
- 增量行情消费、丢包检测和快照同步逻辑
- 策略侧市场订单簿与 BBO 更新
- 特征计算、订单管理、持仓/PnL 跟踪和交易前风控框架
- `RANDOM`、`MAKER`、`TAKER` 三种算法
- 按品种配置下单数量、策略阈值、最大订单量、最大持仓和最大亏损

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
├── trading/                       # 交易客户端侧
│   ├── trading_main.cpp           # 交易客户端程序入口
│   ├── market_data/               # 行情消费、丢包检测与快照恢复
│   ├── order_gw/                  # TCP 订单网关
│   └── strategy/                  # 交易引擎、策略、订单簿、持仓与风控
└── scripts/                       # 构建和本地联调脚本
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

也可以使用项目脚本同时生成 Release 和 Debug 构建：

```bash
bash scripts/build.sh
```

构建产物为 `cmake-build-release/exchange_main` 和 `cmake-build-release/trading_main`。

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

## 运行交易客户端

先启动交易所，再启动一个或多个交易客户端：

```bash
./cmake-build-release/trading_main CLIENT_ID ALGO_TYPE \
  [CLIP THRESH MAX_ORDER_SIZE MAX_POSITION MAX_LOSS] ...
```

每组五个参数对应一个连续的 `TickerId`，第一组对应 `TickerId 0`，最多可配置 8 个品种：

| 参数 | 含义 |
| --- | --- |
| `CLIENT_ID` | 客户端 ID，同时用作随机策略的随机数种子 |
| `ALGO_TYPE` | `RANDOM`、`MAKER` 或 `TAKER` |
| `CLIP` | 策略单次目标订单量 |
| `THRESH` | 策略触发阈值，具体语义取决于算法 |
| `MAX_ORDER_SIZE` | 单笔订单数量上限 |
| `MAX_POSITION` | 绝对持仓上限 |
| `MAX_LOSS` | PnL 风控下限，通常配置为负数 |

`THRESH` 当前在两种策略中的含义不同：

- `MAKER`：公平价格与最优买卖价之间的价格距离阈值。
- `TAKER`：聚合成交量与对手方最优档数量的比例阈值。
- `RANDOM`：不读取逐品种策略配置，可以只传 `CLIENT_ID` 和 `RANDOM`。

例如启动一个配置了两个品种的做市客户端：

```bash
./cmake-build-release/trading_main 1 MAKER \
  100 0.6 150 300 -100 \
  60 0.6 150 300 -100
```

程序分别写入 `trading_main_<CLIENT_ID>.log` 和 `trading_engine_<CLIENT_ID>.log`。持续 60 秒没有订单或行情事件后，客户端会停止。

仓库还提供本地联调脚本：

```bash
# 启动预设的 MAKER、TAKER 和 RANDOM 客户端
bash scripts/run_clients.sh

# 构建并依次启动交易所和所有预设客户端
bash scripts/run_exchange_and_clients.sh
```

## 默认网络配置

| 服务 | 协议 | 地址/接口 | 端口 |
| --- | --- | --- | ---: |
| 订单网关 | TCP | `lo` | `12345` |
| 增量行情 | UDP 组播 | `233.252.14.3` | `20001` |
| 快照行情 | UDP 组播 | `233.252.14.1` | `20000` |

当前交易所及客户端网络配置分别直接定义在 `exchange/exchange_main.cpp` 和 `trading/trading_main.cpp` 中。

## 开发状态与限制

后续仍需重点完善：

- 完善并验证交易端策略、持仓、PnL 与风控链路
- 将 MAKER 的价格阈值和 TAKER 的成交量比例阈值拆分为独立配置项
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
