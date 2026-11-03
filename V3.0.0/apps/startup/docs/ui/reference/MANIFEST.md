# UI Reference Manifest

## 目标环境

| 项目 | 值 | 证据状态 | 来源 / TBD ID |
|---|---|---|---|
| 平台 / OS / Qt / 平台样式 | Windows；模板已在 Qt 6.8.3 MSVC 2022 环境构建 | CONFIRMED（验证环境） | 2026-11-02 本机构建；正式目标见 TBD-002 |
| 目标客户区逻辑尺寸 | 未确认；模板候选最小 960×540 | TBD / INFERRED | TBD-003 |
| OS 缩放 / DPI / DPR | 未确认；PNG 的 96 DPI 标签不证明运行 DPR | TBD | TBD-003 |
| 字体 / 主题 / 语言 | 浅色；英文占位；字体仅有 fallback 候选 | INFERRED / TBD | TBD-003、TBD-008 |
| 测试数据 / 捕获方式 | 空账号/密码；`QWidget::grab()`；Windows QPA | CONFIRMED（本轮） | Capture CAP-001/CAP-002 |

## 图像清单

| Ref ID | 文件 | 来源类型 | 页面 / 角色 / 状态 / 主题 | 图像像素 | 客户区与外框口径 | DPI / DPR | 版本 / 日期 | active / superseded | 替代 Ref | 证据 / TBD |
|---|---|---|---|---:|---|---|---|---|---|---|
| S-01 | `../../../../测氢系统启动缓存时界面.png` | generated-reference | 启动 / 未认证 / 品牌展示 / 浅色 | 1774×887 | 画布；非已知客户区 | DPR UNKNOWN | 文件时间 2026-10-21；SHA-256 `BA5D…A283` | active | N/A | 像素与哈希 CONFIRMED；运行映射 TBD-003 |
| S-02 | `../../../../测氢系统登录界面.png` | generated-reference | 登录 / 未认证 / idle / 浅色 | 1671×941 | 画布；非已知客户区 | DPR UNKNOWN | 文件时间 2026-10-31；SHA-256 `FBE0…EBB` | active | N/A | 像素与哈希 CONFIRMED；运行映射 TBD-003 |

构建使用 `../../../resources/reference/` 内的同哈希副本；原始文件保持不动。

## 校准锚点

| Anchor ID | Ref / 区域 | 已知逻辑尺寸与单位 | 来源 | 证据状态 | 校准用途 / 限制 |
|---|---|---|---|---|---|
| ANC-001 | S-01 / 全画布 | 1774×887 device pixels | PNG 元数据 | CONFIRMED | 只用于比例；不能直接当 Qt 逻辑尺寸 |
| ANC-002 | S-02 / 全画布 | 1671×941 device pixels | PNG 元数据 | CONFIRMED | 只用于比例；不能直接当 Qt 逻辑尺寸 |
| ANC-003 | S-02 / 表单区域 | `ui-map.json` 中估计框 | 视觉测量 | INFERRED | 实控件初始对齐；目标环境必须重校准 |

## 资产来源与使用条件

| 资产 | 原始来源 | 可用性 / 许可证依据 | 采用 / 替代决定 | 关联 TBD |
|---|---|---|---|---|
| 道纪量子 Logo、产品标题、科学插画 | S-01/S-02 用户提供样图 | 本次内部模板使用已获当前请求授权；正式分发许可未提供 | 模板内使用同哈希副本；发布前确认 | TBD-004 |
| 表单图标 | Qt/C++ `QPainter` 代码绘制 | 项目自有代码候选 | 采用 | N/A |
| UI 字体 | 系统 fallback | 未打包字体；原图字体未知 | 当前仅验证环境 fallback | TBD-003 |

## 候选截图

| Capture ID | 路径 | Page / State / Ref | 构建版本 | 客户区 | OS / Qt / 样式 | DPI / DPR / 缩放 | 字体 / 主题 / 语言 | 数据 / 时间 | 捕获方法 |
|---|---|---|---|---|---|---|---|---|---|
| CAP-001 | `../../../artifacts/captures/splash-windows.png` | 启动 / 品牌展示 / S-01 | 0.1.0 Release | 请求的 widget 候选 1500×750；保存 PNG 1875×938 device px | Windows / Qt 6.8.3 / Windows QPA | Qt capture 映射未记录，不能由比值反推正式 DPR | 系统 fallback / 浅色 / 中文品牌图 | 2026-11-02 | `QWidget::grab()` |
| CAP-002 | `../../../artifacts/captures/login-windows-v2.png` | 登录 / idle / S-02 | 0.1.0 Release | 请求的 widget 候选 1504×847；保存 PNG 1880×1055 device px | Windows / Qt 6.8.3 / Windows QPA | Qt capture 映射未记录，不能由比值反推正式 DPR | Microsoft YaHei UI + Segoe UI fallback / 浅色 / 英文表单 | 2026-11-02 | `QWidget::grab()` |

`offscreen` QPA 产生的候选图因字体方框问题保留为诊断证据，不作为视觉验收基准。
