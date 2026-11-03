# TDLAS 管理员 UI Reference Manifest

图片路径相对于模板根目录；原始 `管理员` 目录中的 PNG 未被覆盖。

## 目标环境

| 项目 | 值 | 证据状态 | 来源 / TBD |
|---|---|---|---|
| 平台 / Qt / 样式 | Windows、Qt 6.8.3、Windows QPA（仅本轮） | CONFIRMED | 实际构建/截图 |
| 请求客户区 | 1504×847 logical px candidate | INFERRED | 模板构造函数 |
| 截图文件 | 1880×1055 device px | CONFIRMED | PNG 元数据 |
| OS 缩放 / DPI / DPR | 目标环境未正式确认 | TBD | TBD-A-003 |
| 字体 / 主题 / 语言 | Microsoft YaHei UI/Segoe UI fallback、light、zh-CN | INFERRED | QSS 候选 |
| 测试数据 | 固定模拟、未连接、未校准 | CONFIRMED | 模板实现 |

## 图像清单

| Ref ID | 文件 | 来源类型 | 页面 / 状态 | 图像像素 | 版本 | active | SHA-256 |
|---|---|---|---|---:|---|---|---|
| A-01 | `resources/reference/a01-main.png` | generated/reference raster | 管理员主页面 | 1672×941 | 2026-10-31 | active | `9B8ED536D399FC3C10B1EE17B925A21D7E3B5A8C39F4A9861CDC4336D6C7FB75` |
| A-02 | `resources/reference/a02-main-parameters.png` | generated/reference raster | 主页面 + 参数抽屉 | 1672×941 | 2026-10-31 | active | `8880992D1220C2EED0F0D5C9C9FABB1CDAF53E4E3216E13A3C892E2EFDC277EA` |
| A-03 | `resources/reference/a03-debug.png` | generated/reference raster | 调试 2×2 | 1672×941 | 2026-10-31 | active | `5883A7ABED36B3CACE36A3B59362523784FD5EDB8864BE836938BE83CC1D612B` |
| A-04 | `resources/reference/a04-peak-data.png` | generated/reference raster | 峰值趋势/表格 | 1672×941 | 2026-10-31 | active | `7CF0F3930D0B881A7A0939E7841327E7B20175A23FC81D26C3197B03A40AD47B` |
| A-05 | `resources/reference/a05-camera.png` | generated/reference raster | 摄像默认 | 1671×941 | 2026-10-31 | active | `65BB5BB085705DC304D5D80CEACB5B052FB8970FD3FCD71A4732EB3485498AF8` |
| A-06 | `resources/reference/a06-camera-motion.png` | generated/reference raster | 摄像 + 位移/缩放 | 1671×941 | 2026-10-31 | active | `77A205BD34776B0C75B5E5C4E56C440ECB59C2E0C2CDF0E1923C163F0CEB3A6E` |

生成图无可证明的真实 DPR；客户区、窗口外框和 OS 缩放不能由画布尺寸反推。

## 校准锚点

| Anchor ID | Ref / 区域 | 已知尺寸 | 证据 | 用途 / 限制 |
|---|---|---|---|---|
| ANC-A-001 | A-01～A-04 全画布 | 1672×941 device px | CONFIRMED | 仅用于相对比例 |
| ANC-A-002 | A-05/A-06 全画布 | 1671×941 device px | CONFIRMED | 仅用于相对比例 |
| ANC-A-003 | 共同外壳 | 顶栏/侧栏/页签重复结构 | INFERRED | 不能证明 Qt logical px |

## 资产来源与使用条件

| 资产 | 原始来源 | 当前决定 | TBD |
|---|---|---|---|
| 6 张样图 | 用户目录 `管理员` | 保留原图；模板副本仅用于内部证据，不编入 QRC | TBD-A-004 |
| Logo / 图标 | 样图内可见但无独立源资产 | 模板用文本/符号候选，不从截图裁切 | TBD-A-004/A-010 |
| 相机照片 | 样图内可见但无独立许可资产 | 模板用自绘模拟装置 | TBD-A-004 |

## 候选截图

| Capture ID | 路径 | Page / State / Ref | 构建 | 请求客户区 | 环境 | 数据 |
|---|---|---|---|---:|---|---|
| CAP-A-001 | `artifacts/captures/main.png` | Main / A-01 | Release | 1504×847 | Qt 6.8.3 / Windows QPA | simulated |
| CAP-A-002 | `artifacts/captures/parameters-v2.png` | Parameters / A-02 | Release | 1504×847 | Qt 6.8.3 / Windows QPA | simulated |
| CAP-A-003 | `artifacts/captures/debug.png` | Debug / A-03 | Release | 1504×847 | Qt 6.8.3 / Windows QPA | simulated |
| CAP-A-004 | `artifacts/captures/peak.png` | Peak / A-04 | Release | 1504×847 | Qt 6.8.3 / Windows QPA | simulated |
| CAP-A-005 | `artifacts/captures/camera.png` | Camera / A-05 | Release | 1504×847 | Qt 6.8.3 / Windows QPA | disconnected/simulated |
| CAP-A-006 | `artifacts/captures/camera-motion-v2.png` | Camera motion / A-06 | Release | 1504×847 | Qt 6.8.3 / Windows QPA | disconnected/simulated |
| CAP-A-007 | `artifacts/captures/peak-window-controls-removed.png` | Peak / duplicate-controls fix | Release | 1504×847 | Qt 6.8.3 / Windows QPA | simulated |
