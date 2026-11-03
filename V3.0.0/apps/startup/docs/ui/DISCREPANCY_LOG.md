# UI Discrepancy Log

状态：Open / Investigating / Fix Planned / Fixed / Verified / Accepted / Won't Fix。

## 差异与冲突

| ID | Page / State | Ref | 环境 | 期望 | 实际 | 证据 | 严重度 | 根因类别 | 处理 | Owner | 状态 | 复验结果 / 证据 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| DIF-001 | Login / idle | S-02 | Windows / Qt 6.8.3 | 表单视觉与参考接近且可交互 | 实控件使用不透明浅色表面，和栅格渐变存在轻微块面差异 | CAP-002 | S2 Minor | accepted-constraint | 保留真实控件；在目标环境微调表面与几何 | UX / TBD | Open | 待 TBD-003 关闭后复验 |
| DIF-002 | Login / idle | S-02 | Qt offscreen QPA | 控件文字正常 | offscreen 字体后端将文字渲染为方框 | `login-candidate.png` | S1 Major | platform | 不把 offscreen 图作为验收基准；改用 Windows QPA 隐藏进程捕获 | Build / TBD | Verified | CAP-002 字体正常 |
| DIF-003 | Startup / brand | S-01 | Windows / Qt 6.8.3 | 透明外缘由正式宿主决定 | 模板在 `#F4FAFF` 背景上承载透明 PNG | CAP-001 | S3 Cosmetic | reference-ambiguity | 目标工程确认窗口透明/实色策略 | UX / TBD | Open | 待 TBD-009 |
| DIF-004 | All | S-01/S-02 | 候选尺寸 | 在统一目标客户区逐像素/比例比较 | 正式客户区、DPR、字体和 OS 缩放尚未给出 | MANIFEST | S1 Major | calibration | 当前只判结构可用，不判像素级通过 | Product / UX | Open | 待 TBD-003 |
| DIF-005 | Package / launch | N/A | Windows / Qt 6.8.3 | 用户可从交付目录启动程序 | 直接双击 `build\Release` EXE 报缺少 `Qt6Widgets.dll` | 用户截图 | S0 Blocker | deployment | 新增 `deploy.ps1`，生成含 Qt DLL、Windows 平台插件和 MSVC Runtime 的 `dist` | Build | Verified | 清除 Qt PATH 后 `dist` EXE exit 0，并写出 `deployed-login-final.png` |

## 豁免与接受

当前没有已记录的正式豁免。DIF-001 的不透明实控件策略是模板安全边界，不等于最终视觉豁免。

## 本轮结论

- 范围 / 版本：启动与登录 Codex 编译模板 0.1.0。
- 结论：结构实现与本机编译验证完成；正式视觉验收未完成。
- 已执行：Qt 6.8.3 Release 构建；完整 `dist` 部署；无 Qt PATH 独立启动；Windows QPA 启动/登录截图；逐图检查结构、比例、溢出与字体。
- 未执行：真实目标工程集成、目标 DPI/字体/窗口截图、认证/权限/导航、自动化交互测试。
- 阻塞：TBD-001、TBD-003、TBD-005、TBD-006、TBD-009。
