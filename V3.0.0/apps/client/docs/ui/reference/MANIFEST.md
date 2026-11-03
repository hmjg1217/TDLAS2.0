# TDLAS 客户 UI Reference Manifest

图片路径相对于模板根目录；原始 `客户` 目录中的 PNG 未被覆盖。

## 目标环境

| 项目 | 值 | 证据 | 来源 / TBD |
|---|---|---|---|
| 平台 / Qt / 样式 | Windows、Qt 6.8.3、Windows QPA（仅本轮） | CONFIRMED | 实际构建/截图 |
| 请求窗口候选 | 1504×847 Qt logical px | INFERRED | 模板构造函数 |
| 候选截图文件 | 1880×1055 device px | CONFIRMED | PNG 元数据 |
| OS 缩放 / DPI / DPR | 正式目标环境未确认 | TBD | TBD-C-003 |
| 字体 / 主题 / 语言 | Microsoft YaHei UI/Segoe UI fallback、light、zh-CN | INFERRED | QSS 候选 |
| 测试数据 | 固定模拟、未连接、未校准 | CONFIRMED | 模板实现 |

## 图像清单

| Ref ID | 文件 | 来源类型 | 页面 / 状态 | 图像像素 | 版本 | active | SHA-256 |
|---|---|---|---|---:|---|---|---|
| C-01 | `resources/reference/c01-main.png` | generated/reference raster | 客户主页面 | 1672×941 | 2026-10-31 | active | `055B41394AFCA0E9182C90BE978210A5F473227AB48F85F4E86A3FFFB7207E2E` |
| C-02 | `resources/reference/c02-camera.png` | generated/reference raster | 客户摄像默认态 | 1671×941 | 2026-10-31 | active | `18659BAC8D9B59ED5275BD2C608E6A678DB4F538681CE7484AE8CA9B3F1DD9DA` |
| C-03 | `resources/reference/c03-camera-motion.png` | generated/reference raster | 摄像 + 位移/缩放浮层 | 1670×941 | 2026-10-31 | active | `58E5D5BDF2718053C9BB15936F7A85EA39F6DE89D6E82CDB362CC60388C46508` |

生成图无可证明的真实 DPR；客户区、窗口外框和 OS 缩放不能由画布尺寸反推。

## 校准锚点

| Anchor ID | Ref / 区域 | 已知尺寸 | 证据 | 用途 / 限制 |
|---|---|---|---|---|
| ANC-C-001 | C-01 全画布 | 1672×941 device px | CONFIRMED | 仅用于相对比例 |
| ANC-C-002 | C-02 全画布 | 1671×941 device px | CONFIRMED | 仅用于相对比例 |
| ANC-C-003 | C-03 全画布 | 1670×941 device px | CONFIRMED | 仅用于相对比例 |
| ANC-C-004 | 共同外壳 | 顶栏/侧栏/两页签重复结构 | INFERRED | 不能证明 Qt logical px |

## 资产来源与使用条件

| 资产 | 原始来源 | 当前决定 | TBD |
|---|---|---|---|
| 3 张样图 | 用户目录 `客户` | 保留原图；模板副本仅作内部证据，不编入 QRC | TBD-C-004 |
| Logo / 图标 | 样图内可见但无独立源资产 | 使用文本/基础符号，不从截图裁切 | TBD-C-004/C-010 |
| 设备照片 | 样图内可见但无独立许可资产 | 使用自绘模拟装置 | TBD-C-004 |

## 候选截图

| Capture ID | 路径 | Page / State / Ref | 构建 | 请求尺寸候选 | 文件像素 | 数据 |
|---|---|---|---|---:|---:|---|
| CAP-C-001 | `artifacts/captures/main-v2.png` | Main / C-01 | Release | 1504×847 | 1880×1055 | simulated |
| CAP-C-002 | `artifacts/captures/parameters-v2.png` | Parameters / contract | Release | 1504×847 | 1880×1055 | disconnected/simulated |
| CAP-C-003 | `artifacts/captures/camera-v2.png` | Camera / C-02 | Release | 1504×847 | 1880×1055 | disconnected/simulated |
| CAP-C-004 | `artifacts/captures/camera-motion-v2.png` | Camera motion / C-03 | Release | 1504×847 | 1880×1055 | disconnected/simulated |
| CAP-C-005 | `artifacts/captures/dist-main.png` | deployed Main / C-01 | app-local Release | 1504×847 | 1880×1055 | simulated |
| CAP-C-006 | `artifacts/captures/dist-main-native-titlebar-v3.png` | deployed Main / 图像 1 顶栏复验 | app-local Release | 1504×847 | 1880×1055 | simulated；无重复窗口控件 |
