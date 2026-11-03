# TDLAS Hydrogen UI Suite 差异记录

状态：Open / Investigating / Fix Planned / Fixed / Verified / Accepted / Won't Fix。

## 差异与冲突

本轮整合预览与原目录当前已部署 EXE 的 12 个页面/状态截图 SHA-256 全部一致；整合登录页与独立登录页也完全一致。未发现整合引入的 UI 视觉差异。

## 工程风险处置

| ID | 范围 | 风险 | 处理 | 严重度 | 状态 | 复验 |
|---|---|---|---|---|---|---|
| INT-R-001 | admin/client | ChartWidget 与 CameraPreviewWidget 同名且实现不完全一致 | 分别放入 adminui / clientui；不覆盖、不错误抽取共享实现 | S1 | Verified | 单进程 Release 链接 + GUI CTest |
| INT-R-002 | 四个应用入口 | 三个原 main.cpp 不能与主入口链接进同一 target | 主程序只使用 integrated/main.cpp；原 main 各属预览 target | S1 | Verified | 全目标 Release build |
| INT-R-003 | 单进程角色切换 | 宽泛全局 QSS 可能交叉污染 | QApplication 每次只替换为当前角色 QSS，不拼接 | S1 | Verified | GUI CTest 检查角色 QSS + 截图 |
| INT-R-004 | 旧 build/dist | CMake cache 含原绝对路径，直接搬运不可移植 | 不复制，统一重新配置、构建和部署 | S1 | Verified | configure/build/deploy/run PASS |
| INT-R-005 | 发布包 | 发布独立管理员预览 EXE 会绕过登录 | deploy.ps1 默认只部署 tdlas_hydrogen_ui.exe；预览需显式开关 | S1 | Verified | dist 仅 1 个 EXE |
| INT-R-006 | CTest | build EXE 初次运行找不到 Qt DLL，退出 0xc0000135 | CTest 用 cmake -E env 注入 Qt bin；GUI 使用 offscreen QPA | S1（验证阻塞） | Verified | 2/2 tests PASS |
| INT-R-007 | 侧栏 QRC | 中文源文件名导致 MSVC C4828 编码警告 | 保留原图，RCC 使用逐字节相同的英文文件名副本 | S2 | Verified | 无 C4828 的 Release build；派生哈希核对 |

## 本轮结论

- 范围 / 版本：整合工程 0.2.0，临时登录路由。
- 结论：本轮范围通过；Release build、2 项测试、13 状态截图、12 状态源工程对照、默认部署和 app-local 运行均通过。
- 阻塞本轮的 TBD / S0 / S1：无。
- 未关闭范围：正式认证/授权/会话、目标 DPI/字体/像素级验收、资源发布许可、侧栏图标实际绑定；见 UI_SPEC 的 TBD-INT-002～005。
