# TDLAS 管理员 UI 技术规格

> 整合备注（2026-11-02）：为与客户实现安全共存于单一 executable，本规格中的 AdminTemplateWindow、ChartWidget、CameraPreviewWidget 实际 C++ 符号现位于 adminui 命名空间；objectName、QSS、状态和页面视觉未改变。

## 1. 文档控制与目标

- 项目 / 目标：把 A-01～A-06 管理员样图转换为 Codex 可读、可修改、可编译的 Qt Widgets UI 模板。
- 版本 / 日期 / 负责人：0.1.0 Draft / 2026-11-02 / Codex 候选规格。
- 状态：Draft；未获得产品、UX、设备、算法或安全负责人的正式批准。
- 本次模式：图片转规格 + 独立 UI 模板实施 + 结构级截图检查。
- 包含：共享外壳、主页面、参数抽屉、调试页、峰值页、摄像页、位移/缩放浮层。
- 不包含：认证/权限实现、NI 设备、AO/AI、采集线程、算法、标定、摄像头占用、文件导出、持久化和部署决定。
- 候选平台：Windows、Qt 5/6 Widgets、C++17、CMake；本轮实际验证为 Qt 6.8.3 + MSVC 2022 x64 Release。
- 授权来源：用户要求转换 `管理员` 目录文件并生成 Codex 可理解的样图编译模板。
- 项目边界：遵守上级 `CODEX_UI_WORKFLOW.md`；未发现适用于本目录的 `AGENTS.md`；未复用 `TDLAS_UI_Template`。

证据标记：`CONFIRMED`（直接观察/实际文件/实际运行）、`INFERRED`（可回退候选）、`TBD`（需外部契约）、`N/A`。

## 2. 输入与环境

| 字段 | 值 | 证据状态 | 来源 / TBD ID |
|---|---|---|---|
| Reference Manifest | `docs/ui/reference/MANIFEST.md` | CONFIRMED | 文件存在 |
| 参考图 | A-01～A-06，PNG；1671/1672×941 px | CONFIRMED | 文件元数据与哈希 |
| 目标客户区 | 候选 1504×847；最小候选 1080×680 | INFERRED | 可回退模板实现；TBD-A-003 |
| OS / Qt / 样式 | Windows 11 候选、Qt 6.8.3、Windows QPA | CONFIRMED（仅本轮） | 实际构建与截图 |
| DPI / DPR / 缩放 | 截图文件为 1880×1055 device px；请求窗口 1504×847 logical px，实际映射仍须目标环境确认 | TBD | TBD-A-003 |
| 字体 / 主题 / 语言 | Microsoft YaHei UI/Segoe UI fallback、浅色、简体中文 | INFERRED | QSS 候选；TBD-A-003 |
| 测试数据 | 固定模拟曲线、模拟记录、未连接、未校准 | CONFIRMED（模板） | `ChartWidget.cpp`、`AdminTemplateWindow.cpp` |
| 截图方式 | EXE `--screen ... --capture ...`，Windows QPA | CONFIRMED | 实际执行 |

## 3. 工程审计与不变量

| 项目 | 现状 / 实际路径 / 符号 | 证据 | 本次影响 |
|---|---|---|---|
| 构建系统 | `CMakeLists.txt`，目标 `tdlas_admin_codex_template` | CONFIRMED | 新建独立模板 |
| 主窗口 / 导航 | `AdminTemplateWindow` + `QStackedWidget` | CONFIRMED | 六个预览状态 |
| C++ / QSS / 资源 | `src/`、`resources/styles/admin.qss`、`resources/admin.qrc` | CONFIRMED | 真实控件与自绘组件 |
| 图表 | `ChartWidget` 自绘固定模拟曲线 | CONFIRMED | 正式项目应替换为真实共享模型/图表组件 |
| 摄像预览 | `CameraPreviewWidget` 自绘模拟设备画面 | CONFIRMED | 未申请相机、未处理真实帧 |
| 参考图 | `resources/reference/` 只读副本；未进入 QRC | CONFIRMED | 仅供追溯与人工对照 |
| 业务/设备连接点 | 仅有 `actionRequested(QString)` UI 意图；无外部连接 | CONFIRMED | 保持 UI-only |
| 用户已有修改 | `管理员` 原目录仅有 6 张 PNG；均未覆盖 | CONFIRMED | 新增 `codex-qt-template/` |

必须保持的不变量：

- 不在 UI 构造函数连接设备、相机、网络或执行阻塞 I/O。
- 页面切换不隐式停止采集、清空数据或重连设备。
- 未校准浓度保持 `— / 未校准`；所有候选数据显式标“模拟”。
- 参数保存、采集、复位、导出、拍照、录像和位移/缩放只发 UI 意图，不伪造成功。
- 权限、设备时序、算法、单位、持久化和错误映射由正式业务契约决定。
- 不把完整主工作台截图作为背景，不修改 `ui_*.h` 生成文件。

## 4. 页面与共享组件

| Page ID | 页面 / 角色 | 入口 | 实际类 / 文件 | Ref | 适用状态 | 规格状态 |
|---|---|---|---|---|---|---|
| PAGE-ADMIN-MAIN | 管理员主页面 | F1 / `--screen main` | `AdminTemplateWindow::buildMainPage` | A-01 | simulated-ready | Draft implemented |
| STATE-PARAMETERS | 参数覆盖状态 | F2 / 参数区 / `--screen parameters` | `buildParameterDrawer` | A-02 | open/closed | Draft implemented |
| PAGE-ADMIN-DEBUG | 调试界面 | F3 / 页签 | `buildDebugPage` | A-03 | simulated-ready | Draft implemented |
| PAGE-ADMIN-PEAK | 峰值数据 | F4 / 页签 | `buildPeakPage` | A-04 | simulated-records | Draft implemented |
| PAGE-ADMIN-CAMERA | 摄像界面 | F5 / 页签 | `buildCameraPage` | A-05 | disconnected/simulated | Draft implemented |
| STATE-CAMERA-MOTION | 位移缩放覆盖状态 | F6 / 设置按钮 | `buildMovementPanel` | A-06 | open/closed | Draft implemented |

| Component ID | 职责 | 出现页面 | Qt 类型 | 输入 / 输出 | 证据 |
|---|---|---|---|---|---|
| CMP-SHELL-TOP | 品牌、设备/采集/数据状态、主题/设置；窗口控制由系统标题栏负责 | 全部 | `QFrame` + `QHBoxLayout` | host state / theme-settings intent | CONFIRMED by user + implemented |
| CMP-SHELL-RAIL | 主页面、参数、采集、复位、保存入口 | 全部 | `QPushButton` 列 | click → `actionRequested` | INFERRED + implemented |
| CMP-PAGE-TABS | 四页切换 | 全部 | `QButtonGroup` + `QStackedWidget` | page intent | INFERRED + implemented |
| CMP-CHART | 坐标轴、网格、曲线、模拟状态 | 主/调试/峰值 | `ChartWidget` | host samples / visual | INFERRED + implemented |
| CMP-CAMERA | 预览面板 | 主/摄像 | `CameraPreviewWidget` | host frame / display | INFERRED + simulated |
| CMP-PARAM-DRAWER | 11 项采集参数 | 主页面覆盖 | `QFrame` + `QFormLayout` | fields / save-reset intent | A-02 + implemented |
| CMP-PEAK-TABLE | PeakRecord 明细 | 峰值页 | 模板 `QTableWidget`；正式候选 `QTableView` | model / selection | INFERRED + implemented |
| CMP-MOTION | 显示位移/缩放 | 摄像页覆盖 | 非模态 `QFrame` | pan/zoom intent | A-06 + implemented |

## 5. 页面规格

### 5.1 共享外壳

```text
AdminTemplateWindow
├── TopStatusBar
└── Workspace
    ├── SideActionRail
    └── ContentColumn
        ├── PageTabBar
        └── PageStack
```

- 顶栏候选高 64、侧栏候选宽 88、页签栏候选高 64 Qt logical px（`INFERRED/ESTIMATED`）。
- `QStackedWidget` 仅改变可见页面；模板未实现任何页面切换副作用。
- 应用顶栏不重复创建最小化、最大化或关闭按钮；使用操作系统标题栏自带控件（用户 2026-11-02 确认）。
- F1～F6 可选择六种开发预览状态；Esc 先关闭覆盖层，再关闭窗口。
- 顶栏明确显示“未连接 / 就绪 / 模拟数据 / UI 模板”。

### 5.2 PAGE-ADMIN-MAIN / STATE-PARAMETERS

区域树：

```text
MainPage
├── CameraPreviewPanel
├── FvChartPanel
├── PeakTrendPanel
├── RealtimeResultsPanel
└── AcquisitionParameterDrawer (overlay)
```

| Region | Qt 布局 | stretch / resize | 证据 |
|---|---|---|---|
| 主工作区 | 2×2 `QGridLayout` | 列 13:11；两行等伸缩 | INFERRED/ESTIMATED |
| 参数抽屉 | 根窗口子 `QFrame`，覆盖工作区 | 候选宽 410；高度随窗口 | A-02 + ESTIMATED |
| 结果区 | 3 个语义指标行 | 垂直均分 | A-01 + INFERRED |

参数抽屉的 11 项：采集卡、输出通道、输出波形、波形频率、波形幅值、波形偏移量、采集通道、采样率、显示窗口读取点数、量程上限、量程下限。候选值来自样图，只用于表面复现；设备识别项明确标“未连接”。

结果显示规则：峰值与光强可显示“模拟”，浓度在没有标定契约时固定 `— 未校准`。

### 5.3 PAGE-ADMIN-DEBUG

```text
DebugPage
├── RawSignalChart
├── FitCurveChart
├── FvChart
├── PeakHistoryChart
└── ResultSummaryStrip
```

- 四个图必须保持等宽等高 2×2（`INFERRED`，由 A-03 与项目说明共同支持）。
- 每图具有标题、轴名、单位、数据状态；正式接入时补充图例、缩放复位和错误/空状态。
- 无效数据必须使用空值/断线并说明原因，不填 0、不沿用上一批次。
- 调试峰值历史与峰值页必须使用同一 `PeakRecord` model（正式集成要求；TBD-A-006）。

### 5.4 PAGE-ADMIN-PEAK

```text
PeakDataPage
├── FilterToolbar
├── PeakTrendPanel
│   ├── SummaryLine
│   └── PeakTrendChart
└── PeakRecordTable
```

- 工具栏可见字段：时间范围、run、自动跟随、返回实时、缩放复位、导出 CSV。
- 模板表格使用 10 行稳定模拟数据并允许滚动；正式工程使用共享 `PeakRecord` model/view。
- 不同 `runId` 不连线；空/全无效批次用 null 断线；重复批次不重复追加；乱序结果按 sequence 归位。
- 排序、过滤、分页、导出字段、时区、算法版本和标定版本均由正式数据契约提供（TBD-A-006）。
- 缩放交互存在文档冲突，模板不实现自由缩放（TBD-A-009）。

### 5.5 PAGE-ADMIN-CAMERA / STATE-CAMERA-MOTION

```text
CameraPage
├── PreviewAndCaptureColumn
│   ├── CameraPreview
│   └── PhotoRecordActions
├── CameraControlColumn
│   ├── DeviceAndFormatControls
│   ├── ImageParameterControls
│   └── MediaList
└── MotionZoomPanel (overlay)
```

- 页面候选列伸缩比 5:3（`ESTIMATED`）。
- 进入页面不申请摄像头；只有正式“开启预览”用例可以申请设备（TBD-A-007）。
- 摄像与测量采集状态独立；一侧错误不得改变另一侧状态。
- 位移/缩放只表达画面显示变换，不表示实体镜头移动。
- 模板中的设备、媒体、曝光、增益和滑块为真实控件，但不会读写设备或磁盘。

### 5.6 状态与事件边界

| State / Event | 进入或触发 | 模板反馈 | 正式退出/成功/失败 | 证据 |
|---|---|---|---|---|
| simulated-ready | 模板启动 | 显示模拟曲线与未连接状态 | N/A | CONFIRMED template |
| parameters-open | 参数区/F2 | 抽屉覆盖主页面 | 关闭/Esc | A-02 |
| camera-motion-open | 设置/F6 | 浮层覆盖控制区 | 关闭/Esc | A-06 |
| start/stop/reset/save | 侧栏按钮 | 顶栏提示“仅发出 UI 意图” | 正式用例 TBD | TBD-A-005 |
| export/capture/record | 页面按钮 | 顶栏提示未接入 | 正式用例 TBD | TBD-A-006/007 |
| pan/zoom | 浮层按钮 | 顶栏提示显示意图 | host transform TBD | TBD-A-007 |

键盘与无障碍候选：按钮和输入框使用原生 Tab 顺序；Esc 关闭覆盖层；真实应用需补全 `accessibleName`、tooltip、非颜色状态、快捷键冲突和屏幕阅读器测试。

## 6. 导航与角色

| From | Event | Guard / 来源 | To | 不允许情况 | 证据 |
|---|---|---|---|---|---|
| 任一管理员页 | 页签选择 | 正式可信权限/会话层 | 目标页 | 仅通过隐藏按钮实现权限 | TBD-A-008 |
| 主页面 | 参数区 | UI 可用且业务允许编辑 | STATE-PARAMETERS | 抽屉打开导致采集中断 | A-02 + INFERRED |
| 摄像页 | 位移/缩放设置 | 预览 UI 可用 | STATE-CAMERA-MOTION | 推断物理镜头动作 | A-06 |

管理员入口和调试页可见性不由本模板决定；正式工程必须复用既有会话、角色、构建开关和审计策略。

## 7. 设计令牌

- 候选事实源：`docs/ui/DESIGN_TOKENS.md` 与 `resources/styles/admin.qss`。
- 正式工程已有主题时，以正式主题为唯一事实源并映射本规格。
- 图标与 Logo 为候选文本符号；正式资产、许可与高 DPI 机制待确认。

## 8. TBD 与决定记录

| TBD ID | 位置 / 问题 | 所需证据 / 决定 | 阻塞 | 临时策略 | 状态 |
|---|---|---|---|---|---|
| TBD-A-001 | 正式目标工程、入口、分支、AGENTS 未提供 | 目标 repo 和接入点 | 正式集成 | 独立可删除模板 | Open |
| TBD-A-002 | 正式 Qt/编译器/构建系统 | 支持矩阵或目标工程配置 | 正式构建 | Qt 5/6 CMake；本机 Qt 6.8.3 验证 | Open |
| TBD-A-003 | 客户区、DPI/DPR、缩放、字体、最小尺寸 | 统一目标环境 | 像素级验收 | 相对布局 + 1504×847 候选 | Open |
| TBD-A-004 | Logo、图标、样图与相机素材许可 | 资产来源与分发许可 | 发布 | 参考图仅内部证据，不编入 QRC | Open |
| TBD-A-005 | NI/采集/参数验证、默认值、状态与错误 | 设备与 application 接口 | 真实采集 | 只发 UI 意图 | Open |
| TBD-A-006 | PeakRecord、算法、单位、有效性、标定和导出契约 | 稳定数据模型/用例 | 真实曲线、浓度、CSV | 固定模拟/未校准 | Open |
| TBD-A-007 | 摄像头所有权、格式、参数范围、文件生命周期 | camera use-case 接口 | 真实摄像 | 自绘模拟预览，不申请设备 | Open |
| TBD-A-008 | 管理员/调试权限与会话失效行为 | 可信权限契约 | 正式导航 | 模板不判断权限 | Open |
| TBD-A-009 | 峰值缩放交互冲突 | UX/Product 决定 | 交互验收 | 仅保留按钮表面 | Open |
| TBD-A-010 | 正式 SVG/图标、中文字体和品牌细节 | UX/Brand 资产 | 视觉验收/发布 | Unicode 候选符号 | Open |

## 9. 工作包与修改边界

| Work Package | Page / Ref | 目标 | allowlist | 不变量 | 验证 |
|---|---|---|---|---|---|
| WP-A-01-SPEC | A-01～A-06 | ARX + Qt 规格、映射、TBD | `docs/` | 不改原图 | JSON parse、路径/哈希 |
| WP-A-02-SHELL | 全部 | 共享外壳与页面导航 | `src/`、QSS、CMake | UI-only | Release build |
| WP-A-03-PAGES | A-01～A-06 | 四页与两个覆盖状态 | `src/`、QSS | 模拟/未连接/未校准 | 六状态截图 |
| WP-A-04-INTEGRATE | 全部 | 接入正式工程 | TBD | 不改变设备/算法/权限契约 | TBD-A-001～010 |

回退方式：删除 `管理员/codex-qt-template/`；上级原 PNG 不受影响。

## 10. 验证与截图矩阵

| VER ID | 类型 | 命令 / 操作 | 环境 | 通过标准 | 实际结果 |
|---|---|---|---|---|---|
| VER-A-001 | configure/build | `.\build.ps1 -QtRoot C:\Qt\6.8.3\msvc2022_64` | Windows / VS 2022 / Qt 6.8.3 | exit 0，生成 Release EXE | PASS，2026-11-02 |
| VER-A-002 | render | EXE `--screen <state> --capture <png>` | Windows QPA | 六状态 exit 0、PNG 非空 | PASS |
| VER-A-003 | structure QA | 人工查看六张候选图 | 1880×1055 device px capture | 无结构缺失、拉伸、主要溢出或中文缺字 | PASS（结构级） |
| VER-A-004 | deploy/run | `.\deploy.ps1 ...`；运行 `dist` EXE 截图 | app-local Qt DLL / no SDK PATH injection | exit 0、写出 PNG | PASS |
| VER-A-007 | duplicate window controls | 删除应用顶栏三个 `windowAction`，重建、部署并截图 | Windows native title bar | 客户区不再出现重复控件；系统标题栏机制不变 | PASS，2026-11-02 |
| VER-A-005 | target visual QA | 正式工程与校准环境 | TBD-A-001/003/010 | 无 S0/S1 | NOT RUN |
| VER-A-006 | device/business regression | 正式接口与硬件环境 | TBD-A-005/006/007/008 | 契约和回归通过 | NOT RUN |

| Capture ID | Page / State / Ref | 请求客户区候选 | 路径 | 比较结果 |
|---|---|---:|---|---|
| CAP-A-001 | Main / A-01 | 1504×847 | `artifacts/captures/main.png` | 结构 CLOSE；模拟画面替代照片，图标为候选 |
| CAP-A-002 | Parameters / A-02 | 1504×847 | `artifacts/captures/parameters-v2.png` | 11 项控件齐全，底层不重排 |
| CAP-A-003 | Debug / A-03 | 1504×847 | `artifacts/captures/debug.png` | 2×2 等分和底部摘要齐全 |
| CAP-A-004 | Peak / A-04 | 1504×847 | `artifacts/captures/peak.png` | 工具栏、趋势、表格齐全；小高度表格滚动 |
| CAP-A-005 | Camera / A-05 | 1504×847 | `artifacts/captures/camera.png` | 预览/控制分栏齐全，全部模拟/未连接 |
| CAP-A-006 | Camera motion / A-06 | 1504×847 | `artifacts/captures/camera-motion-v2.png` | 非模态浮层齐全 |
| CAP-A-007 | Peak / duplicate-controls fix | 1504×847 | `artifacts/captures/peak-window-controls-removed.png` | 应用顶栏仅保留主题/设置；重复的最小化、最大化、关闭控件已移除 |

## 11. 差异、确认与交付

- 差异日志：`docs/ui/DISCREPANCY_LOG.md`。
- 已关闭：初始 Qt 6 `QButtonGroup` 信号编译差异；初始命令行 camera-motion 截图未在布局完成后重置浮层，均已修改并复验。
- 未关闭：正式字体/图标/品牌、真实相机照片、目标 DPI、业务状态、设备/算法/权限和缩放契约。
- 本次完成：规格、参考清单、坐标映射、Qt 5/6 CMake 工程、四页、两个覆盖状态、Release 构建、六状态结构截图。
- allowlist 外变化：后续同步上级 `CODEX_UI_WORKFLOW.md` 的管理员批次状态；不修改原图或 `TDLAS_UI_Template`。
- 结论：独立 UI 模板可编译并完成结构级候选验收；正式工程接入、真实功能和像素级验收未完成。
