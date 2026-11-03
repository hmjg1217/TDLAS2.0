# TDLAS 客户 UI Discrepancy Log

状态：Open / Investigating / Fix Planned / Fixed / Verified / Accepted / Won't Fix。

## 差异与冲突

| ID | Page / State | Ref | 期望 | 模板实际 | 严重度 | 根因 | 处理 | 状态 |
|---|---|---|---|---|---|---|---|---|
| DIF-C-001 | All | C-01～C-03 | 稳定可读图标 | 首轮部分 Unicode 候选符号依赖字体 | S2 Minor | asset/platform | 改用稳定文本/基础符号并重截 v2 | Verified |
| DIF-C-002 | Main/Camera | C-01～C-03 | 批准的装置照片 | 自绘模拟装置 | S2 Minor | asset/license | 保持模拟；正式资产待 TBD-C-004 | Open |
| DIF-C-003 | Exact geometry/type | C-01～C-03 | 像素级一致 | 正式客户区、DPR、字体未确认 | S1 Major | calibration | 统一目标环境后校准 | Open |
| DIF-C-004 | Live status/value | C-01～C-03 | 不误导真实状态 | 样图显示已连接/采集中/浓度 | S0 risk if copied | data/safety | 模板显示未连接、模拟、未校准 | Verified |
| DIF-C-005 | Client authorization | C-01～C-03 | 客户仅访问允许页面 | 只有可见页签证据 | S1 Major | spec-gap | 正式可信权限层决定 | Open |
| DIF-C-006 | Parameter drawer | written contract | 客户参数入口明确 | 三张客户图未展示该状态 | S1 Major | reference-ambiguity | 保留可回退候选，待 TBD-C-009 | Open |
| DIF-C-007 | Shared top bar / Main | 用户标注图像 1 | Windows 原生标题栏负责最小化、最大化、关闭 | 应用顶栏重复提供三项窗口按钮 | S2 Minor | duplicated shell controls | 删除控件、信号连接与 `windowAction` QSS；保留默认原生窗口框架 | Verified |

## 豁免与接受

无正式豁免或接受记录。DIF-C-002/003/005/006 不能自动视为已接受。

## 本轮结论

- 范围：独立 Qt UI 模板 0.1.0。
- 结构级结论：四种状态可渲染，无结构缺失、拉伸、主要溢出或中文缺字；DIF-C-001/004/007 已复验关闭。
- 最终结论：未完成正式视觉/业务验收，因为 TBD-C-001/003/005～010 未关闭。
- 已执行：Release build、四状态 Windows QPA 截图、逐图人工检查、app-local 部署运行、JSON 解析、原图/副本 SHA-256 与 QRC 边界校验。
- 未执行：正式工程集成、目标 DPI/字体对照、真实权限、设备/算法/摄像/保存回归。
