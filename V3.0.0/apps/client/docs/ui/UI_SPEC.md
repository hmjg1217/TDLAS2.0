# TDLAS 客户 Qt Widgets UI 技术规格

> 整合备注（2026-11-02）：为与管理员实现安全共存于单一 executable，本规格中的 ClientTemplateWindow、ChartWidget、CameraPreviewWidget 实际 C++ 符号现位于 clientui 命名空间；objectName、QSS、状态和页面视觉未改变。

## 1 文档控制与目标

- 版本：0.1.0 Draft，2026-11-02。
- 模式：按参考图实现独立 UI-only 编译模板。
- 页面：客户主页面、客户摄像页面、摄像位移/缩放浮层；另含书面契约要求的参数抽屉状态。
- 技术候选：C++17、Qt 5/6 Widgets、CMake；本机使用 Qt 6.8.3 + MSVC 2022 验证。
- 不包含：登录/会话、可信权限、真实 NI/AI/AO、采集线程、算法、标定、摄像头、文件保存、导出、照片、录像和持久化。
- 用户授权：把 `客户` 目录样图转成 Codex 可理解且可编译的模板。
- 项目约束：原始 PNG 不覆盖；整页样图不作为功能背景；`TDLAS_UI_Template` 未作为源码、QSS、对象名、参数或工程结构来源。

证据使用 `CONFIRMED / INFERRED / TBD / N/A`；ARX JSON 另使用 `EXACT / INFERRED / ESTIMATED / UNKNOWN`。

## 2 输入与环境

| 字段 | 值 | 证据 | 来源 / TBD |
|---|---|---|---|
| Reference Manifest | `docs/ui/reference/MANIFEST.md` | CONFIRMED | 模板文件 |
| 样图 | C-01 1672×941；C-02 1671×941；C-03 1670×941 device px | CONFIRMED | PNG 元数据 |
| 样图性质 | 设计参考栅格，不是已校准应用截图 | CONFIRMED | 文件与任务语境 |
| 模板请求尺寸 | 1504×847 Qt logical px candidate | INFERRED | 构造函数，可回退 |
| 模板最小尺寸 | 1080×680 Qt logical px candidate | INFERRED | 结构候选 |
| 本轮平台 | Windows / Qt 6.8.3 / Windows QPA / MSVC 2022 | CONFIRMED | 实际构建与截图 |
| 候选截图尺寸 | 1880×1055 device px | CONFIRMED | PNG 元数据 |
| 正式 DPI / DPR / OS 缩放 | 未确认 | TBD | TBD-C-003 |
| 字体 | Microsoft YaHei UI / Segoe UI fallback；精确源字体未知 | INFERRED | QSS / TBD-C-010 |
| 测试数据 | 固定模拟、未连接、未校准 | CONFIRMED | 模板实现 |

## 3 工程审计与不变量

| 项目 | 现状 | 证据 | 本次影响 |
|---|---|---|---|
| 正式目标工程 | 未提供 | TBD-C-001 | 独立可删除模板 |
| 构建系统 | 新建 CMake；Qt Widgets only | CONFIRMED | `CMakeLists.txt` |
| 主窗口 / 导航 | `ClientTemplateWindow` + `QStackedWidget` | CONFIRMED | 两个客户页签 |
| UI / QSS / 资源 | C++ layouts、自绘图表/预览、单一作用域 QSS | CONFIRMED | 无 `.ui` 生成文件 |
| 业务 / 设备连接点 | 未接入；按钮只显示 UI 意图 | CONFIRMED | 不操作硬件/磁盘 |
| 用户已有修改 | `客户` 原先仅有 3 张 PNG | CONFIRMED | 原图保持不变 |

不变量：UI 不直接调用设备 SDK，不伪造连接/采集/保存/拍照/录像成功，不把未校准浓度显示为真实 ppm；页面切换不停止采集、不清空宿主数据、不重连设备；客户页面可见性不替代可信权限检查。

## 4 页面与共享组件

| Page / State ID | 名称 | Ref | 入口 | 实现 | 状态 |
|---|---|---|---|---|---|
| PAGE-C-MAIN | 客户主页面 | C-01 | 默认 / 主页面页签 | `buildMainPage()` | CONFIRMED |
| STATE-C-PARAMETERS | 参数抽屉覆盖态 | 书面设计契约 | 左栏参数区 | `buildParameterDrawer()` | INFERRED |
| PAGE-C-CAMERA | 客户摄像页 | C-02 | 摄像界面页签 | `buildCameraPage()` | CONFIRMED |
| STATE-C-CAMERA-MOTION | 摄像位移缩放态 | C-03 | 位移/缩放设置 | `buildMovementPanel()` | CONFIRMED |

| Component ID | 职责 | Qt 类型 | 输入 / 输出 | 证据 |
|---|---|---|---|---|
| CMP-C-SHELL | 顶栏、侧栏、两页签、内容栈；最小化/最大化/关闭由 Windows 原生标题栏提供 | `QWidget/QFrame/QStackedWidget` | 页面选择意图 | INFERRED |
| CMP-C-CHART | FV / 峰值趋势展示 | 自绘 `QWidget` | 宿主数据 + 有效性 | INFERRED |
| CMP-C-PREVIEW | 摄像画面表面 | 自绘/视频 `QWidget` | 摄像帧 + 摄像状态 | INFERRED |
| CMP-C-RESULTS | 三项实时结果 | `QFrame/QLabel` | 值、单位、有效性、来源 | INFERRED |
| CMP-C-DRAWER | 11 项参数覆盖抽屉 | `QFrame/QFormLayout/QScrollArea` | 编辑值 / 保存意图 | 书面契约 |
| CMP-C-MOTION | 位移/缩放非模态浮层 | `QFrame/QGridLayout` | 显示变换意图 | C-03 |

## 5 页面规格

### 5.1 PAGE-C-MAIN

区域树：`ClientMainPage → CameraPreviewPanel + FvChartPanel + PeakTrendPanel + RealtimeResultsPanel`。使用 2×2 `QGridLayout`，页面边距候选 16、卡片间距候选 12 Qt logical px。列 stretch 5:6，行 stretch 6:5，仅为可回退实现值。

| Region | 内容 | SizePolicy / resize | 数据边界 |
|---|---|---|---|
| MAIN-CAMERA | 预览、模拟标识、画面格式 | Expanding；保持可读预览 | 摄像状态与采集状态独立 |
| MAIN-FV | FV 曲线、坐标轴、单位、数据状态 | Expanding | 无效数据断线，不填 0 |
| MAIN-PEAK | 峰值随时间趋势 | Expanding | 值来自完整有效 FV 批次 |
| MAIN-RESULTS | 峰值吸光度、光强百分比、气体浓度 | Expanding | 浓度未校准显示“— / 未校准” |

模板固定值 `0.0978` 与 `78.65 %` 明确带“模拟”；不作为算法输出。正式工程必须由展示模型提供数值、单位、有效性、批次与时间。

### 5.2 STATE-C-PARAMETERS

覆盖主页面左侧，不触发底层网格重排。字段固定 11 项：采集卡、输出通道、输出波形、波形频率、波形幅值、波形偏移量、采集通道、采样率、显示窗口读取点数、量程上限、量程下限。字段值是布局候选，不创建硬件任务；正式范围、枚举、单位、默认值和锁定条件由 TBD-C-005 关闭。

### 5.3 PAGE-C-CAMERA

区域树：`ClientCameraPage → PreviewAndCaptureColumn + CameraControlColumn`。主列与控制列 stretch 5:3；左侧为预览和拍照/录像动作，右侧为设备格式、相机参数与媒体列表。

| Region | 可见内容 | 行为边界 |
|---|---|---|
| CAMERA-PREVIEW | 大预览、模拟状态、格式/FPS | 进入页面不自动申请摄像头 |
| CAMERA-CAPTURE | 拍照、开始/停止录像、时长 | 只发意图；模板不写媒体文件 |
| CAMERA-DEVICE | 相机枚举、分辨率、帧率、位移设置 | 运行中格式变更策略 TBD |
| CAMERA-PARAMS | 曝光、自动曝光、亮度、对比度、饱和度 | 范围与能力由设备契约决定 |
| CAMERA-MEDIA | 图片/视频缩略项、打开目录 | 目录、覆盖、编码、错误 TBD |

### 5.4 STATE-C-CAMERA-MOTION

位移/缩放面板覆盖右侧控制区但不阻塞预览，不重新排版底层页面。上/下/左/右与放大/缩小只改变显示画布；除非另有批准的设备契约，不解释为实体镜头运动。Esc 或关闭按钮返回摄像默认态。

### 5.5 状态、事件与键盘

| Event | 前置 | 模板即时反馈 | 正式成功/失败 |
|---|---|---|---|
| 页面切换 | UI 可用 | 切换 `QStackedWidget` | 不改变采集/摄像会话 |
| 开始/停止采集 | 可信应用层允许 | 顶栏提示“仅 UI 意图” | TBD-C-005 |
| 参数保存 | 表单值可提交 | 提示“未写入配置” | TBD-C-005 |
| 拍照/录像 | 摄像会话可用 | 提示未申请设备/未写文件 | TBD-C-007 |
| 位移/缩放 | 预览获得焦点/面板可用 | 提示仅改变模拟画布 | TBD-C-007 |
| 数据保存 | 会话数据存在 | 提示未写磁盘 | TBD-C-006 |

F1～F4 分别选择主页面、参数抽屉、摄像页、摄像浮层；Esc 先关闭覆盖层，再关闭模板窗口。Tab 使用原生控件顺序。正式应用需补齐 `accessibleName`、tooltip、焦点顺序、快捷键冲突、非颜色状态和屏幕阅读器验证。

## 6 导航与角色

| From | Event | Guard / 来源 | To | 不允许情况 |
|---|---|---|---|---|
| 任一客户页 | 主页面页签 | 正式会话/权限层 | PAGE-C-MAIN | 仅隐藏按钮代替授权 |
| 任一客户页 | 摄像页签 | 正式会话/权限层 | PAGE-C-CAMERA | 页面进入即申请摄像头 |
| 主页面 | 参数区 | 正式业务允许编辑 | STATE-C-PARAMETERS | 抽屉打开导致采集中断 |
| 摄像页 | 位移/缩放设置 | 预览 UI 可用 | STATE-C-CAMERA-MOTION | 推断物理镜头动作 |

样图只证明客户壳层可见两个页签，不能证明角色枚举、授权条件、会话失效或管理员入口规则。

## 7 设计令牌

- 候选事实源：`docs/ui/DESIGN_TOKENS.md` 与 `resources/styles/client.qss`。
- 正式工程已有主题时，以正式主题为唯一事实源并映射本规格。
- 参考图内 Logo、图标与设备照片没有独立许可资产；模板不从截图裁切。

## 8 TBD 与决定记录

| TBD ID | 问题 | 所需证据 / 决定 | 阻塞 | 临时策略 | 状态 |
|---|---|---|---|---|---|
| TBD-C-001 | 正式目标工程、入口、分支、AGENTS 未提供 | 目标 repo 与接入点 | 正式集成 | 独立可删除模板 | Open |
| TBD-C-002 | 正式 Qt/编译器/构建系统 | 正式工程配置 | 正式构建 | Qt 5/6 CMake；Qt 6.8.3 本机验证 | Open |
| TBD-C-003 | 客户区、DPI/DPR、缩放、字体、最小尺寸 | 统一目标环境 | 像素级验收 | 相对布局 + 1504×847 候选 | Open |
| TBD-C-004 | Logo、SVG、样图与设备照片许可 | 批准资产与分发许可 | 发布 | 文本/自绘模拟替代 | Open |
| TBD-C-005 | NI/采集/11 项参数验证、默认值和状态 | 设备与 application 接口 | 真实采集 | 只发 UI 意图 | Open |
| TBD-C-006 | 算法、单位、有效性、标定、PeakRecord 与保存契约 | 稳定模型/用例 | 真实曲线与结果 | 固定模拟/未校准 | Open |
| TBD-C-007 | 摄像头所有权、格式、参数范围、编码和文件生命周期 | camera use-case 接口 | 真实摄像 | 自绘模拟，不申请设备 | Open |
| TBD-C-008 | 客户/管理员权限与会话失效行为 | 可信权限契约 | 正式导航 | 模板不判断权限 | Open |
| TBD-C-009 | 客户参数抽屉是否正式可用 | Product/UX 与权限决定 | 参数交互验收 | 保留书面契约候选态 | Open |
| TBD-C-010 | 正式字体、品牌和高 DPI 图标 | UX/Brand 资产 | 视觉验收/发布 | 系统字体 + 稳定文本 | Open |

## 9 工作包与修改边界

| Work Package | 范围 | allowlist | 不变量 | 验证 |
|---|---|---|---|---|
| WP-C-01-SPEC | ARX + Qt 规格、映射、TBD | `docs/` | 不改原图 | JSON parse / 哈希 |
| WP-C-02-SHELL | 客户共享外壳、两页导航 | `src/`、QSS、CMake | UI-only | Release build |
| WP-C-03-PAGES | 主页面、参数、摄像、浮层 | `src/`、QSS | 模拟/未连接/未校准 | 四状态截图 |
| WP-C-04-INTEGRATE | 接入正式工程 | TBD | 不改变设备/算法/权限契约 | TBD-C-001～010 |

回退方式：删除 `客户/codex-qt-template/`；上级三张原 PNG 不受影响。

## 10 验证与截图矩阵

| VER ID | 类型 | 命令 / 操作 | 环境 | 实际结果 |
|---|---|---|---|---|
| VER-C-001 | configure/build | `.\build.ps1 -QtRoot C:\Qt\6.8.3\msvc2022_64` | Windows / VS 2022 / Qt 6.8.3 | PASS，Release EXE，2026-11-02 |
| VER-C-002 | render | EXE `--screen <state> --capture <png>` | Windows QPA | PASS，四状态 PNG 非空 |
| VER-C-003 | structure QA | 人工查看四张 v2 候选图 | 1880×1055 device px | PASS，无结构缺失、拉伸、主要溢出或中文缺字 |
| VER-C-004 | deploy/run | `deploy.ps1`；无 Qt SDK PATH 运行 `dist` EXE | app-local Qt DLL | PASS，exit 0 并写出 `dist-main.png` |
| VER-C-005 | JSON / hash | PowerShell 解析 JSON、校验参考副本 | 本机文件系统 | PASS；两份 JSON 可解析，三份参考副本与原图 SHA-256 一致，QRC 不含 PNG |
| VER-C-006 | target visual QA | 正式工程与校准环境 | TBD-C-001/003/010 | NOT RUN |
| VER-C-007 | device/business regression | 正式接口/硬件环境 | TBD-C-005～008 | NOT RUN |
| VER-C-008 | native-titlebar cleanup | 静态检索、Release 构建、无 Qt SDK PATH 运行部署 EXE 并截图 | Windows / app-local Qt DLL | PASS；应用顶栏无重复窗口控件，默认原生窗口框架保留 |

| Capture | Page / State / Ref | 请求尺寸候选 | 文件 | 结构结论 |
|---|---|---:|---|---|
| CAP-C-001 | Main / C-01 | 1504×847 | `artifacts/captures/main-v2.png` | CLOSE；模拟画面替代照片 |
| CAP-C-002 | Parameters / written contract | 1504×847 | `artifacts/captures/parameters-v2.png` | 11 项齐全，底层不重排 |
| CAP-C-003 | Camera / C-02 | 1504×847 | `artifacts/captures/camera-v2.png` | CLOSE；未连接/模拟替代真实状态 |
| CAP-C-004 | Camera motion / C-03 | 1504×847 | `artifacts/captures/camera-motion-v2.png` | CLOSE；非模态浮层可见 |
| CAP-C-006 | Main / 图像 1 顶栏复验 | 1504×847 | `artifacts/captures/dist-main-native-titlebar-v3.png` | PASS；应用顶栏仅保留主题与设置 |

## 11 差异、确认与交付

- 差异日志：`docs/ui/DISCREPANCY_LOG.md`。
- 已关闭：首轮不稳定 Unicode 候选图标在部分控件呈方框，已替换为稳定文本/基础符号并复验。
- 未关闭：正式字体/图标/品牌、批准的设备照片、目标 DPI、真实业务状态、设备/算法/权限/摄像契约和客户参数入口。
- 本次完成：规格、参考清单、坐标映射、Qt 5/6 CMake 工程、两页与两个覆盖状态、Release 构建、部署与四状态结构截图。
- 结论：独立 UI 模板可编译并完成结构级候选验收；正式工程接入、真实功能与像素级验收未完成。
