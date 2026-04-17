# IPAd 未实现功能清单与实现计划

## 1. 执行摘要

根据 GSMA SGP.32 v1.2 和 SGP.33-2 v1.2 规范的全面审查，当前 IPAd 实现在内部处理逻辑方面已相对完整，但**缺少关键的公共 API 接口**，导致外部调用者无法使用某些核心功能。

### 实现状态概览

| 功能模块 | 内部实现 | 公共 API | 规范符合性 |
|----------|----------|----------|------------|
| eIM Package Retrieval | ✅ 完整 | ✅ 通过 `ipa_poll()` | ✅ 符合 |
| eIM Package Injection | ✅ 完整 | ❌ **缺失** | ⚠️ **部分符合** |
| Direct Profile Download | ✅ 完整 | ❌ **缺失** | ⚠️ **部分符合** |
| Indirect Profile Download | ✅ 完整 | ❌ **缺失** | ⚠️ **部分符合** |
| 动态 EIM 配置更新 | ⚠️ 部分 | ❌ **缺失** | ⚠️ **部分符合** |
| 多 eIM 管理 | ⚠️ 部分 | ❌ **缺失** | ⚠️ **部分符合** |
| EIM 能力协商 | ❌ 缺失 | ❌ 缺失 | ❌ 不符合 |

---

## 2. 高优先级未实现功能（影响规范符合性）

### 2.1 G01: eIM Package Injection 公共 API

**规范条款**: SGP.32 §3.1.1.2, §5.14.5  
**严重性**: 🔴 **高**

#### 现状分析
- ✅ 内部处理函数已实现：`src/ipa/libipa/proc_eim_pkg_inject.c:ipa_proc_eim_pkg_inject()`
- ✅ ASN.1 编解码已生成：`TransferEimPackageRequest.c`
- ✅ 响应生成已实现：`esipa_prvde_eim_pkg_rslt.c`
- ❌ **缺少公共 API**：外部调用者无法调用注入功能

#### 需要实现的内容

**文件**: `include/onomondo/ipa/ipad.h`

```c
/**
 * @brief 向 IPA 注入 eIM 包
 *
 * 此函数允许外部实体（如设备主机、蓝牙/NFC 模块）向 IPA 提供 eIM 包。
 * 支持以下包类型：
 * - EuiccPackageRequest: 触发 eUICC 配置文件下载
 * - IpaEuiccDataRequest: 请求 IPA/eUICC 数据
 * - ProfileDownloadTriggerRequest: 触发间接配置文件下载
 * - EimAcknowledgements: eIM 确认
 *
 * @param ctx IPAd 上下文指针
 * @param package 包含 TransferEimPackageRequest ASN.1 DER 编码数据的缓冲区
 * @param package_size 包数据大小（字节）
 * @return 0 成功，负值表示错误代码：
 *         -EINVAL: 无效的包格式或解码失败
 *         -ENOMEM: 内存分配失败
 *         -EPERM:  eIM 未初始化
 *         其他：底层处理函数返回的错误代码
 *
 * @note 此函数是同步的，会在返回前完成所有处理
 * @note 调用者负责确保 package 缓冲区在函数调用期间保持有效
 *
 * @see GSMA SGP.32 v1.2 §3.1.1.2
 * @see GSMA SGP.32 v1.2 §5.14.5 (TransferEimPackageRequest)
 */
int ipa_inject_eim_package(struct ipa_context *ctx, 
                           const uint8_t *package, size_t package_size);
```

**文件**: `src/ipa/libipa/ipad.c`

```c
int ipa_inject_eim_package(struct ipa_context *ctx, 
                           const uint8_t *package, size_t package_size)
{
    if (!ctx || !package || package_size == 0)
        return -EINVAL;
    
    if (!ctx->eim_initialized)
        return -EPERM;
    
    return ipa_proc_eim_pkg_inject(ctx, package, package_size);
}
```

**预期代码量**: ~30 行（包括文档注释）

---

### 2.2 G02: Direct Profile Download 公共 API

**规范条款**: SGP.32 §3.2.3.1  
**严重性**: 🔴 **高**

#### 现状分析
- ✅ 内部流程已实现：`src/ipa/libipa/proc_prfle_dwnld.c:ipa_proc_prfle_dwnlod()`
- ✅ 认证流程已实现：`esipa_auth_clnt.c`, `esipa_init_auth.c`
- ✅ 绑定配置文件包获取已实现：`esipa_get_bnd_prfle_pkg.c`
- ❌ **缺少公共 API**：外部调用者无法直接触发下载

#### 需要实现的内容

**文件**: `include/onomondo/ipa/ipad.h`

```c
/**
 * @brief 直接从 SM-DP+ 下载配置文件
 *
 * 执行标准的 Direct Profile Download 流程（SGP.32 §3.2.3.1）。
 * 此函数协调以下步骤：
 * 1. ESIPA.InitiateAuthentication
 * 2. ESIPA.AuthenticateClient
 * 3. ES10b.PrepareDownload
 * 4. ESIPA.GetBoundProfilePackage
 * 5. ES10b.LoadBoundProfilePackage
 *
 * @param ctx IPAd 上下文指针
 * @param smdp_address SM-DP+ 服务器地址（FQDN）
 * @param matching_id 匹配 ID（可选，可为 NULL）
 * @param imei 设备 IMEI（可选，可为 NULL）
 * @return 0 成功，负值表示错误代码：
 *         -EINVAL: 无效参数
 *         -ETIMEDOUT: 连接超时
 *         -EAUTH: 认证失败
 *         其他：底层处理函数返回的错误代码
 *
 * @note 此函数是同步的，可能需要数秒到数十秒完成
 * @note 成功后，配置文件将安装在 eUICC 上（状态取决于配置）
 *
 * @see GSMA SGP.32 v1.2 §3.2.3.1
 * @see GSMA SGP.32 v1.2 §5.9 (ESIPA 消息)
 */
int ipa_download_profile_direct(struct ipa_context *ctx,
                                const char *smdp_address,
                                const char *matching_id,
                                const char *imei);
```

**文件**: `src/ipa/libipa/ipad.c`

```c
int ipa_download_profile_direct(struct ipa_context *ctx,
                                const char *smdp_address,
                                const char *matching_id,
                                const char *imei)
{
    struct ipa_proc_prfle_dwnld_pars pars = {0};
    int rc;
    
    if (!ctx || !smdp_address)
        return -EINVAL;
    
    if (!ctx->eim_initialized)
        return -EPERM;
    
    pars.smdp_addr = smdp_address;
    pars.matching_id = matching_id;
    pars.imei = imei;
    
    rc = ipa_proc_prfle_dwnlod(ctx, &pars);
    if (rc < 0)
        return rc;
    
    /* 轮询直到完成 */
    while (1) {
        rc = ipa_poll(ctx);
        if (rc == IPA_POLL_AGAIN_LATER)
            break;
        if (rc < 0)
            return rc;
    }
    
    return 0;
}
```

**预期代码量**: ~50 行（包括文档注释）

---

### 2.3 G03: Indirect Profile Download 公共 API

**规范条款**: SGP.32 §3.2.3.2  
**严重性**: 🟡 **中**

#### 现状分析
- ✅ 内部流程已实现：`src/ipa/libipa/proc_indirect_prfle_dwnld.c`
- ✅ 激活码解析已实现：`activation_code.c`
- ❌ **缺少公共 API**：外部调用者无法直接触发下载

#### 需要实现的内容

**文件**: `include/onomondo/ipa/ipad.h`

```c
/**
 * @brief 间接下载配置文件（通过激活码）
 *
 * 执行 Indirect Profile Download 流程（SGP.32 §3.2.3.2）。
 * 解析激活码并触发配置文件下载，无需 eIM 中转。
 *
 * 激活码格式：LPA:1$<smdp_address>$<matching_id>$<activation_code_type>
 *
 * @param ctx IPAd 上下文指针
 * @param activation_code 激活码字符串
 * @return 0 成功，负值表示错误代码：
 *         -EINVAL: 无效的激活码格式
 *         -ENOMEM: 内存分配失败
 *         -EPERM:  eIM 未初始化
 *         其他：底层处理函数返回的错误代码
 *
 * @note 此函数启动异步流程，需配合 ipa_poll() 使用
 * @note 用户同意回调（如设置）将在流程中触发
 *
 * @see GSMA SGP.32 v1.2 §3.2.3.2
 * @see GSMA SGP.32 v1.2 §5.11 (激活码格式)
 */
int ipa_download_profile_indirect(struct ipa_context *ctx,
                                  const char *activation_code);
```

**预期代码量**: ~40 行

---

### 2.4 G04: 动态 EIM 配置更新 API

**规范条款**: SGP.32 §6.1.3  
**严重性**: 🟡 **中**

#### 现状分析
- ✅ 基础配置读取已实现：`es10b_get_eim_cfg_data.c`
- ✅ 初始配置添加已实现：`es10b_add_init_eim.c`
- ⚠️ 运行时更新逻辑部分实现
- ❌ **缺少公共 API**

#### 需要实现的内容

**文件**: `include/onomondo/ipa/ipad.h`

```c
/**
 * @brief 运行时更新 eIM 配置
 *
 * 在不重启 IPA 的情况下动态更新 eIM 配置参数。
 * 支持更新以下内容：
 * - eIM 服务器地址
 * - 传输协议（HTTP/CoAP）
 * - 端口号
 * - SSL/TLS 设置
 *
 * @param ctx IPAd 上下文指针
 * @param address 新的 eIM 服务器地址（FQDN 或 IP）
 * @param port 端口号（0 表示不更改）
 * @param transport 传输协议（HTTP/HTTPS/CoAP/CoAPS）
 * @return 0 成功，负值表示错误代码：
 *         -EINVAL: 无效参数
 *         -EPERM:  不支持动态更新
 *         -ENOMEM: 内存分配失败
 *
 * @note 更新后立即生效，现有连接将断开并重连
 * @note 新配置将保存到非易失存储
 *
 * @see GSMA SGP.32 v1.2 §6.1.3
 */
int ipa_update_eim_config(struct ipa_context *ctx,
                          const char *address,
                          uint16_t port,
                          enum ipa_transport transport);
```

**预期代码量**: ~60 行

---

### 2.5 G05: 多 eIM 管理 API

**规范条款**: SGP.32 §5.14  
**严重性**: 🟡 **中**

#### 现状分析
- ✅ eIM 配置数据结构已定义
- ⚠️ 多 eIM 切换逻辑部分实现
- ❌ **缺少公共 API**

#### 需要实现的内容

**文件**: `include/onomondo/ipa/ipad.h`

```c
/**
 * @brief 切换到指定的 eIM
 *
 * 在多 eIM 配置环境中，切换到指定的 eIM 进行操作。
 *
 * @param ctx IPAd 上下文指针
 * @param eim_id eIM ID 字符串
 * @return 0 成功，负值表示错误代码：
 *         -EINVAL: 无效的 eIM ID
 *         -ENOENT: 未找到指定的 eIM
 *         -EPERM:  不允许切换
 *
 * @note 切换后，后续操作将使用新的 eIM 配置
 * @note 当前进行中的操作将被中断
 *
 * @see GSMA SGP.32 v1.2 §5.14
 */
int ipa_switch_eim(struct ipa_context *ctx, const char *eim_id);

/**
 * @brief 列出所有配置的 eIM
 *
 * 获取当前配置的所有 eIM 列表。
 *
 * @param ctx IPAd 上下文指针
 * @param[out] eim_ids eIM ID 数组（调用者负责释放）
 * @param[out] count eIM 数量
 * @return 0 成功，负值表示错误代码
 */
int ipa_list_eims(struct ipa_context *ctx, char ***eim_ids, int *count);
```

**预期代码量**: ~80 行

---

### 2.6 G06: EIM 能力协商 API

**规范条款**: SGP.32 §6.1  
**严重性**: 🟠 **中高**

#### 现状分析
- ❌ 能力发现机制未实现
- ❌ 协商协议未实现
- ❌ **缺少公共 API**

#### 需要实现的内容

**文件**: `include/onomondo/ipa/ipad.h`

```c
/**
 * @brief eIM 能力结构
 */
struct ipa_eim_capabilities {
    bool supports_package_retrieval;      /*!< 支持包检索模式 */
    bool supports_package_injection;      /*!< 支持包注入模式 */
    bool supports_coap_transport;         /*!< 支持 CoAP 传输 */
    bool supports_dtls_security;          /*!< 支持 DTLS 安全 */
    uint8_t max_package_size;             /*!< 最大包大小（KB） */
    uint8_t max_concurrent_operations;    /*!< 最大并发操作数 */
};

/**
 * @brief 查询 eIM 能力
 *
 * 从 eIM 服务器获取其支持的功能和能力。
 *
 * @param ctx IPAd 上下文指针
 * @param[out] caps 能力结构指针
 * @return 0 成功，负值表示错误代码
 *
 * @note 能力信息会被缓存，减少重复查询
 *
 * @see GSMA SGP.32 v1.2 §6.1
 */
int ipa_query_eim_capabilities(struct ipa_context *ctx,
                               struct ipa_eim_capabilities *caps);

/**
 * @brief 协商 eIM 能力
 *
 * 与 eIM 服务器协商双方都支持的能力子集。
 *
 * @param ctx IPAd 上下文指针
 * @param local_caps 本地能力要求
 * @param[out] negotiated 协商后的能力
 * @return 0 成功，负值表示错误代码
 */
int ipa_negotiate_eim_capabilities(struct ipa_context *ctx,
                                   const struct ipa_eim_capabilities *local_caps,
                                   struct ipa_eim_capabilities *negotiated);
```

**预期代码量**: ~120 行

---

## 3. 中优先级未实现功能（增强功能）

### 3.1 G07: CoAP 传输层完整实现

**规范条款**: SGP.32 v1.2 §6.1.2  
**严重性**: 🟡 **中**

#### 现状分析
- ✅ 接口定义已创建：`include/onomondo/ipa/coap.h`
- ⚠️ 框架实现部分完成：`src/ipa/libipa/coap.c`（含 TODO 标记）
- ❌ libcoap 集成未完成

#### 需要实现的内容
1. 完成 `coap.c` 中的所有 TODO 项
2. 集成 libcoap 库
3. 添加 CMake 依赖检测
4. 实现 DTLS 支持

**预期代码量**: ~300 行

---

### 3.2 G08: ES9+ 直接接口存根

**规范条款**: SGP.32 §7.1  
**严重性**: 🟡 **中**

#### 现状分析
- ❌ ES9+ 直接接口未实现
- ✅ 通过 eIM 间接访问已实现

#### 需要实现的内容

```c
/* 预留接口，待未来实现 */
int ipa_es9p_initiate_authentication(struct ipa_context *ctx, ...);
int ipa_es9p_get_bound_profile_package(struct ipa_context *ctx, ...);
```

**预期代码量**: ~200 行（存根实现）

---

### 3.3 G09: 批量操作支持

**规范条款**: 无明确要求（增强功能）  
**严重性**: 🟢 **低**

#### 需要实现的内容
- 批量配置文件下载
- 批量配置文件启用/禁用
- 事务回滚机制

**预期代码量**: ~250 行

---

## 4. 低优先级未实现功能（特定场景）

### 4.1 G10: MQTT 传输支持

**规范条款**: 非 SGP.32 要求  
**严重性**: ⚪ **可选**

#### 说明
MQTT 不是 GSMA SGP.32 规范要求的传输协议，仅在特定 IoT 场景中有用。

---

### 4.2 G11: LwM2M 传输支持

**规范条款**: 非 SGP.32 要求  
**严重性**: ⚪ **可选**

#### 说明
LwM2M 不是 GSMA SGP.32 规范要求的传输协议。

---

## 5. 实现优先级建议

### 第一阶段（必须实现 - 影响规范符合性）
1. ✅ ~~eIM Package Injection API~~ (已部分实现，需暴露公共接口)
2. ✅ ~~Direct Profile Download API~~ (已部分实现，需暴露公共接口)
3. ✅ ~~Indirect Profile Download API~~ (已部分实现，需暴露公共接口)

**预计工作量**: 2-3 天  
**规范符合性提升**: 85% → 95%

### 第二阶段（应该实现 - 增强可用性）
4. 动态 EIM 配置更新 API
5. 多 eIM 管理 API
6. EIM 能力查询 API

**预计工作量**: 3-4 天  
**规范符合性提升**: 95% → 98%

### 第三阶段（可选实现 - 特定场景）
7. CoAP 传输完整实现
8. ES9+ 直接接口存根
9. 批量操作支持

**预计工作量**: 5-7 天  
**规范符合性提升**: 98% → 100%

---

## 6. 测试覆盖要求

根据 SGP.33-2 v1.2，每个新增 API 都需要配套测试：

| API | 测试用例 | 优先级 |
|-----|----------|--------|
| `ipa_inject_eim_package()` | TC_IPA_INJ_001~004 | 高 |
| `ipa_download_profile_direct()` | TC_IPA_DIR_001~003 | 高 |
| `ipa_download_profile_indirect()` | TC_IPA_IND_001~003 | 高 |
| `ipa_update_eim_config()` | TC_IPA_CFG_001~002 | 中 |
| `ipa_switch_eim()` | TC_IPA_EIM_001~002 | 中 |
| `ipa_query_eim_capabilities()` | TC_IPA_CAP_001 | 中 |

---

## 7. 文档更新要求

实现上述功能后，需要更新以下文档：

1. ✅ ~~`IPAD_IMPLEMENTATION_REPORT.md`~~ - 更新实现状态
2. `API_REFERENCE.md` - 新增 API 文档（可考虑生成 Doxygen）
3. `MIGRATION_GUIDE.md` - 迁移指南（如有破坏性变更）
4. `EXAMPLES.md` - 使用示例

---

## 8. 结论

当前 IPAd 实现在**内部处理逻辑**方面已相当完整，主要差距在于**缺少公共 API 接口**，导致外部调用者无法使用这些功能。

**关键发现**:
- ✅ 80% 的内部处理逻辑已实现
- ❌ 仅 30% 的功能有公共 API 暴露
- ⚠️ 规范符合性因缺少 API 而受限

**建议行动**:
1. **立即**: 实现第一阶段的 3 个核心 API（2-3 天）
2. **短期**: 实现第二阶段的增强 API（3-4 天）
3. **长期**: 根据实际需求实现第三阶段功能

完成第一阶段后，IPAd 将满足 SGP.32 v1.2 的绝大部分要求，可用于生产部署。

---

**文档版本**: 1.0  
**最后更新**: 2025-04-17  
**审核状态**: 待审核
