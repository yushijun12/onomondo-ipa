# IoT Profile Assistant (IPAd) 详细设计文档

## 1. 概述

### 1.1 项目简介
onomondo-ipa 是一个基于 C 语言的 IoT Profile Assistant (IPAd) 实现，符合 GSMA SGP.31 和 SGP.32 规范。它在 IoT 设备中作为 eUICC 和 eIM (eSIM IoT Manager) 之间的中间层，通过 HTTPS 与 eIM 通信，通过智能卡接口与 eUICC 通信。

### 1.2 主要功能
- 实现 ESipa 接口（HTTP 客户端，ASN.1 编码）
- 实现 ES10x 接口（与 eUICC 通信）
- 支持 IoT eUICC 仿真模式（可使用消费级 eUICC）
- 配置文件下载、安装、启用、禁用
- eIM 配置管理
- 通知处理

### 1.3 支持的规范
- GSMA SGP.32 v1.0 (IoT eSIM 规范)
- GSMA SGP.22 (消费级 eSIM 规范，用于仿真模式)

## 2. 系统架构

### 2.1 整体架构图

```
┌─────────────────────────────────────────────────────────────┐
│                      应用层 (Application)                     │
│                    (main.c - 示例应用)                        │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                   IPAd 核心层 (libipa)                        │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐  │
│  │  ipad.c     │  │  esipa.c    │  │  euicc.c/es10x.c    │  │
│  │  上下文管理  │  │  eIM 通信   │  │  eUICC 通信/仿真     │  │
│  └─────────────┘  └─────────────┘  └─────────────────────┘  │
│  ┌─────────────────────────────────────────────────────┐    │
│  │              流程处理模块 (proc_*.c)                  │    │
│  │  proc_eim_pkg_retr.c  - eIM 包检索                    │    │
│  │  proc_euicc_pkg_dwnld_exec.c - eUICC 包下载执行      │    │
│  │  proc_prfle_inst.c    - 配置文件安装                 │    │
│  │  proc_cmn_cancel_sess.c - 会话取消                   │    │
│  │  ...                                                 │    │
│  └─────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────┘
                              │
        ┌─────────────────────┼─────────────────────┐
        ▼                     ▼                     ▼
┌───────────────┐   ┌───────────────┐   ┌─────────────────┐
│  平台抽象层    │   │  ASN.1 编解码  │   │   工具模块       │
│  (Platform)   │   │  (libasn)     │   │   (utils)       │
├───────────────┤   ├───────────────┤   ├─────────────────┤
│ http.c        │   │ 自动生成代码   │   │ activation_code │
│ scard.c       │   │ (ASN.1 定义)   │   │ bpp_segments    │
│ log.c         │   │ 200+ 文件      │   │ length          │
└───────────────┘   └───────────────┘   └─────────────────┘
        │                     │                     │
        ▼                     ▼                     ▼
┌───────────────┐   ┌───────────────┐   ┌─────────────────┐
│ libcurl       │   │               │   │ 标准 C 库        │
│ libpcsclite   │   │               │   │                 │
└───────────────┘   └───────────────┘   └─────────────────┘
```

### 2.2 目录结构

```
/workspace/
├── include/onomondo/ipa/     # 公共头文件
│   ├── ipad.h               # 主 API 接口
│   ├── http.h               # HTTP 客户端接口
│   ├── scard.h              # 智能卡接口
│   ├── log.h                # 日志接口
│   ├── mem.h                # 内存管理宏
│   ├── utils.h              # 工具函数
│   └── http_hdr.h           # HTTP 头部处理
├── src/ipa/
│   ├── libipa/              # IPAd 核心库
│   │   ├── ipad.c           # 上下文管理和主流程
│   │   ├── context.h        # 上下文结构定义
│   │   ├── esipa*.c/h       # ESIPA 协议实现
│   │   ├── es10*.c/h        # ES10x 接口实现
│   │   ├── euicc.c/h        # eUICC 操作封装
│   │   ├── proc_*.c/h       # 业务流程处理
│   │   └── utils*.c/h       # 内部工具函数
│   ├── libasn/              # ASN.1 编解码库（自动生成）
│   ├── http.c               # HTTP 实现（基于 libcurl）
│   ├── scard.c              # 智能卡实现（基于 PC/SC）
│   ├── main.c               # 示例应用程序
│   └── CMakeLists.txt
├── tests/                   # 测试用例
│   ├── activation_code/     # 激活码解析测试
│   ├── bpp_segments/        # BPP 分段测试
│   └── utils/               # 工具函数测试
├── asn1/                    # ASN.1 规范文件
└── contrib/                 # 辅助文件
```

## 3. 核心模块设计

### 3.1 上下文管理 (ipad.c)

#### 3.1.1 数据结构

**ipa_config** - 配置结构
```c
struct ipa_config {
    char *preferred_eim_id;      // 首选 eIM ID
    uint8_t tac[IPA_LEN_TAC];    // 终端类型分配码
    const char *eim_cabundle;    // CA 证书束路径
    bool eim_disable_ssl;        // 禁用 SSL
    bool eim_disable_ssl_verif;  // 禁用 SSL 证书验证
    unsigned int esipa_req_retries;  // eIM 请求重试次数
    bool refresh_flag;           // 刷新标志
    unsigned int reader_num;     // 读卡器编号
    uint8_t euicc_channel;       // 逻辑通道号
    bool iot_euicc_emu_enabled;  // IoT eUICC 仿真开关
    ipa_prfle_inst_consent_cb prfle_inst_consent_cb; // 已弃用
};
```

**ipa_context** - 运行时上下文
```c
struct ipa_context {
    struct ipa_config *cfg;      // 配置指针
    void *http_ctx;              // HTTP 子上下文
    void *scard_ctx;             // 智能卡子上下文
    uint8_t eid[IPA_LEN_EID];    // eUICC ID 缓存
    char *eim_id;                // eIM ID 缓存
    char *eim_fqdn;              // eIM 地址缓存
    struct ipa_proc_eucc_pkg_dwnld_exec_res *proc_eucc_pkg_dwnld_exec_res;
    struct ipa_nvstate nvstate;  // 非易失状态
    bool check_scard;            // 智能卡错误标记
    bool check_http;             // HTTP 错误标记
};
```

**ipa_nvstate** - 非易失状态
```c
struct ipa_nvstate {
    uint32_t version;            // 版本号（用于兼容性检查）
    struct {
        int association_token_counter;
        struct ipa_buf *eim_cfg_ber;
        struct {
            bool flag;
            struct ipa_buf *smdp_oid;
            struct ipa_buf *smdp_address;
        } auto_enable;
    } iot_euicc_emu;
} __attribute__((packed));
```

#### 3.1.2 核心 API

| 函数 | 描述 |
|------|------|
| `ipa_new_ctx()` | 创建新上下文，加载非易失状态 |
| `ipa_init()` | 初始化上下文（读取 eUICC 信息） |
| `ipa_poll()` | 主轮询函数，处理 eIM 包 |
| `ipa_close()` | 关闭上下文 |
| `ipa_free_ctx()` | 释放上下文，保存非易失状态 |

#### 3.1.3 状态机

```
创建上下文 (ipa_new_ctx)
    │
    ▼
初始化 (ipa_init) ──► 读取 EID, eIM 配置
    │
    ▼
轮询循环 (ipa_poll)
    │
    ├─► 检索 eIM 包
    ├─► 执行 eIM 操作
    ├─► 处理通知
    │
    ▼
关闭 (ipa_close / ipa_free_ctx)
    │
    ▼
保存非易失状态
```

### 3.2 ESIPA 协议层 (esipa.c)

ESIPA 是 IPAd 与 eIM 之间的通信协议，基于 HTTP/HTTPS，使用 ASN.1 编码。

#### 3.2.1 消息类型

**从 IPAd 到 eIM:**
- `initiateAuthenticationRequest` - 发起认证
- `authenticateClientRequest` - 客户端认证
- `getBoundProfilePackageRequest` - 获取绑定配置文件包
- `cancelSessionRequest` - 取消会话

**从 eIM 到 IPAd:**
- `initiateAuthenticationResponse` - 认证响应
- `authenticateClientResponse` - 客户端认证响应
- `provideEimPackageResult` - eIM 包结果
- `euiccPackageRequest` - eUICC 包请求

#### 3.2.2 认证流程

```
IPAd                          eIM
 │                             │
 │── initiateAuthentication ──▶│
 │                             │
 │◀─────── auth challenge ─────│
 │                             │
 │── authenticateClient ──────▶│
 │                             │
 │◀─────── auth token ─────────│
 │                             │
 │── getBoundProfilePackage ──▶│
 │                             │
 │◀─────── BPP / Error ────────│
```

### 3.3 eUICC 接口层 (euicc.c, es10x.c)

#### 3.3.1 ES10x 函数分类

**ES10a (管理功能):**
- `GetEuiccConfiguredAddresses` - 获取配置的地址
- `SetNickname` - 设置昵称

**ES10b (配置文件管理):**
- `GetEid` - 获取 EID
- `GetEuiccInfo` - 获取 eUICC 信息
- `PrepareDownload` - 准备下载
- `LoadBoundProfilePackage` - 加载绑定配置文件包
- `EnableProfile` - 启用配置文件
- `DisableProfile` - 禁用配置文件
- `DeleteProfile` - 删除配置文件
- `ListNotification` - 列出通知
- `RetrieveNotificationsList` - 检索通知列表
- `RemoveNotificationFromList` - 移除通知
- `MemoryReset` - 内存重置
- `CancelSession` - 取消会话

**ES10c (信息查询):**
- `GetEid` - 获取 EID
- `GetProfileInfo` - 获取配置文件信息

#### 3.3.2 IoT eUICC 仿真

当使用消费级 eUICC 时，IPAd 提供仿真层来模拟 IoT eUICC 功能：

| IoT 功能 | 消费级替代方案 |
|----------|----------------|
| EIM 配置存储 | 本地仿真 (nvstate) |
| Auto-Enable | ES10b EnableUsingDD |
| 通知管理 | 直接 APDU 命令 |

### 3.4 业务流程模块 (proc_*.c)

#### 3.4.1 eIM 包检索 (proc_eim_pkg_retr.c)

负责从 eIM 获取并处理 eIM 包：
1. 调用 `euicc_package_request`
2. 处理响应（下载、安装、切换等）
3. 返回结果给轮询器

#### 3.4.2 eUICC 包下载执行 (proc_euicc_pkg_dwnld_exec.c)

处理完整的配置文件下载和安装流程：
1. 准备下载（认证、密钥协商）
2. 获取绑定配置文件包 (BPP)
3. 加载 BPP 到 eUICC
4. 发送安装结果通知

#### 3.4.3 配置文件安装 (proc_prfle_inst.c)

处理单个配置文件的安装：
1. 解析 BPP
2. 调用 eUICC 加载函数
3. 处理安装结果

#### 3.4.4 通知传递 (proc_notif_delivery.c)

将 eUICC 生成的通知发送到 eIM：
1. 检索通知列表
2. 构建通知消息
3. 发送到 eIM
4. 移除已发送的通知

## 4. 内存管理设计

### 4.1 内存分配策略

IPAd 使用自定义内存管理宏，支持调试模式下的内存追踪：

```c
// 分配单个对象
#define IPA_ALLOC(obj) IPA_ALLOC_N(sizeof(obj))

// 分配 N 字节
#define IPA_ALLOC_N(n) malloc(n)  // 或带调试的版本

// 分配并清零
#define IPA_CALLOC(nmemb, n) calloc(nmemb, n)

// 重新分配
#define IPA_REALLOC(obj, n) realloc(obj, n)

// 释放
#define IPA_FREE(obj) free(obj)
```

### 4.2 内存调试模式

启用 `MEM_EMIT_DEBUG` 后，所有内存操作都会被追踪：
- 记录当前分配的总字节数
- 记录峰值内存使用量
- 每次分配/释放都打印日志

### 4.3 ipa_buf 缓冲区管理

`ipa_buf` 是 IPAd 中通用的动态缓冲区结构：

```c
struct ipa_buf {
    size_t len;        // 分配的总长度
    size_t data_len;   // 实际数据长度
    uint8_t data[];    // 柔性数组成员
};
```

操作函数：
- `ipa_buf_alloc(size)` - 分配空缓冲区
- `ipa_buf_alloc_data(len, data)` - 分配并复制数据
- `ipa_buf_realloc(buf, new_len)` - 重新分配
- `ipa_buf_free(buf)` - 释放缓冲区
- `ipa_buf_deserialize(data, len)` - 反序列化

## 5. 错误处理机制

### 5.1 轮询返回码

```c
enum ipa_poll_rc {
    IPA_POLL_AGAIN = 0,           // 立即再次轮询
    IPA_POLL_AGAIN_LATER = 1,     // 稍后轮询
    IPA_POLL_AGAIN_WHEN_ONLINE = 2, // 网络恢复后轮询
    IPA_POLL_CHECK_SCARD = -1000, // 智能卡错误
    IPA_POLL_CHECK_HTTP = -2000,  // HTTP 错误
};
```

### 5.2 错误传播链

```
底层 (APDU/HTTP) 
    │
    ▼
中层 (es10x/esipa) ──► 返回错误码
    │
    ▼
高层 (proc_*) ───────► 记录日志，更新状态
    │
    ▼
API (ipa_poll) ──────► 返回轮询码给调用者
```

## 6. 外部依赖分析

### 6.1 当前依赖

| 依赖 | 用途 | 可选项 |
|------|------|--------|
| libcurl | HTTPS 通信 | 是（需替换） |
| libpcsclite | 智能卡访问 | 是（需替换） |
| asn1c | ASN.1 编解码 | 否（已生成代码） |
| C99 标准库 | 基础功能 | 否 |

### 6.2 依赖优化方向

1. **HTTP 层抽象**: 提供更轻量的 HTTP 客户端接口
2. **智能卡层抽象**: 支持多种智能卡访问方式
3. **ASN.1 优化**: 减少生成代码的内存占用

## 7. 测试架构

### 7.1 现有测试

| 测试模块 | 描述 |
|----------|------|
| activation_code_test | 激活码解析测试 |
| bpp_segments_test | BPP 分段处理测试 |
| utils_test | 工具函数测试 |

### 7.2 测试覆盖目标

- [ ] 核心 API 单元测试
- [ ] ESIPA 协议测试
- [ ] eUICC 仿真测试
- [ ] 内存泄漏检测
- [ ] 集成测试套件

## 8. 性能优化建议

### 8.1 RAM 优化

1. 减少动态分配，使用静态缓冲区
2. 优化 ASN.1 解码器的内存使用
3. 及时释放不再使用的缓冲区

### 8.2 Flash 优化

1. 移除未使用的 ASN.1 类型编解码器
2. 优化日志输出代码
3. 使用编译器优化选项

### 8.3 代码优化

1. 统一错误处理模式
2. 减少代码重复
3. 提高模块化程度

## 9. 未来扩展方向

### 9.1 规范升级

- 支持 SGP.32 v1.2+
- 支持 SGP.33 (IoT eSIM 远程管理)

### 9.2 功能增强

- 多 eUICC 支持
- 并发会话处理
- 离线模式支持

### 9.3 平台移植

- RTOS 适配（FreeRTOS, Zephyr）
- 裸机环境支持
- 硬件安全模块集成

## 10. 附录

### 10.1 缩略语

| 缩写 | 全称 |
|------|------|
| IPAd | IoT Profile Assistant device |
| eUICC | embedded Universal Integrated Circuit Card |
| eIM | eSIM IoT Manager |
| BPP | Bound Profile Package |
| ASN.1 | Abstract Syntax Notation One |
| PC/SC | Personal Computer/Smart Card |

### 10.2 参考文档

- GSMA SGP.31: IoT eSIM Architecture
- GSMA SGP.32: IoT eSIM Specification
- GSMA SGP.22: Consumer eSIM Specification
- ISO/IEC 7816: Smart Card Standards
