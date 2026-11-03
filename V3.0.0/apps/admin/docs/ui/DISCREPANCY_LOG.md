# TDLAS 管理员 UI Discrepancy Log

状态：Open / Investigating / Fix Planned / Fixed / Verified / Accepted / Won't Fix。

## 差异与冲突

| ID | Page / State | Ref | 环境 | 期望 | 实际 | 严重度 | 根因 | 处理 | 状态 / 复验 |
|---|---|---|---|---|---|---|---|---|---|
| DIF-A-001 | Build | N/A | Qt 6.8.3 | Qt 5/6 信号连接可编译 | 初版使用已移除的 `buttonClicked(int)` 重载 | S1 Major | platform/API | 使用 Qt 5.15+/Qt 6 `idClicked`，旧 Qt 使用 `QOverload` | Verified；Release build exit 0 |
| DIF-A-002 | Camera motion initial capture | A-06 | Windows QPA | 命令行状态显示浮层 | 初次截图时布局完成后浮层层级被页面子控件覆盖 | S1 Major | layout/state | `show()` 后重设请求状态并 `raise()` | Verified；`camera-motion-v2.png` |
| DIF-A-003 | Camera imagery | A-01/A-05/A-06 | Template | approved chamber photo | 模板使用自绘模拟装置 | S2 Minor | asset/license | 保持模拟并明确标识；正式资产和许可待 TBD-A-004 | Open |
| DIF-A-004 | Icons / brand detail | 全部 | Windows QPA | 与样图图标一致 | 当前为 Unicode 候选符号，部分符号随字体变化 | S2 Minor | asset/platform | 正式 SVG 和字体待 TBD-A-010 | Open |
| DIF-A-005 | Exact geometry / typography | 全部 | 候选 1504×847 | 像素级一致 | 目标客户区、DPR、字体未确认 | S1 Major | calibration | 在统一目标环境校准 | Open，TBD-A-003 |
| DIF-A-006 | Peak zoom behavior | Peak | A-04 + 项目说明 | 单一明确交互 | 项目资料存在冲突 | S1 Major | reference-ambiguity | 模板不实现隐藏缩放语义，待 UX/Product 决定 | Open，TBD-A-009 |
| DIF-A-007 | Live values/status | Main/Debug/Peak/Camera | 全部 | 不误导真实状态 | 样图展示连接/采集/浓度 | S0 risk if copied | data/safety | 模板改为未连接、模拟、未校准 | Verified in six captures |
| DIF-A-008 | Shared top bar | User implementation screenshot, 2026-11-02 | Windows native title bar | 窗口控制只出现一次 | 应用顶栏重复显示最小化、最大化、关闭按钮 | S2 Minor | layout/spec-gap | 删除三个 `windowAction` 及其信号连接和 QSS | Verified；Release build、deploy 与 `peak-window-controls-removed.png` 复验通过 |

## 豁免与接受

无正式豁免或接受记录。DIF-A-003～006 不能自动视为已接受。

## 本轮结论

- 范围：独立 Qt UI 模板 0.1.0。
- 结构级结论：六种状态可渲染，无结构缺失、拉伸、主要溢出或中文缺字；DIF-A-001/002 已复验关闭。
- 最终结论：未完成正式视觉/业务验收，因为 TBD-A-001/003/005～010 未关闭。
- 已执行：Release build、六状态 Windows QPA 截图、逐图人工结构检查、JSON 解析（交付前执行）。
- 未执行：正式工程集成、目标 DPI/字体对照、设备/算法/权限/摄像回归、真实导出与持久化。
