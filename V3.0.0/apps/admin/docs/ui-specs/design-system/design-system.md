# TDLAS 管理员界面设计系统规格

## Source

- 模式：`system`
- 参考：A-01 至 A-06，共 6 张桌面栅格样图；副本位于 `resources/reference/`，SHA-256 见 `design-system.json`。
- 画布：A-01 至 A-04 为 1672×941 px，A-05/A-06 为 1671×941 px（`EXACT`）。
- 限制：画布像素不能证明 Qt 客户区、DPR、Windows 缩放、字体或响应式断点。

## Summary

管理员界面由持续可见的顶栏、左侧动作栏、顶部页面页签和主工作区组成。四个业务页面分别是主页面、调试界面、峰值数据和摄像界面；参数抽屉与位移/缩放面板是覆盖状态，不应改变底层页面布局。

模板使用真实 Qt 控件、布局、自绘曲线和模拟摄像画面。六张整图只用于证据追溯，不参与运行时界面绘制。

## Component Tree

```text
AdminTemplateWindow
├── TopStatusBar
└── Workspace
    ├── SideActionRail
    └── ContentColumn
        ├── PageTabBar
        └── PageStack
            ├── MainPage
            │   ├── CameraPreviewPanel
            │   ├── FvChartPanel
            │   ├── PeakTrendPanel
            │   ├── RealtimeResultsPanel
            │   └── AcquisitionParameterDrawer (overlay)
            ├── DebugPage (2×2 charts + summary)
            ├── PeakDataPage (filters + trend + table)
            └── CameraPage
                ├── PreviewAndCaptureColumn
                ├── CameraControlColumn
                └── MotionZoomPanel (overlay)
```

## Layout

- 顶栏、侧栏和页签条保持持续可见（`INFERRED`）。
- 最小化、最大化和关闭仅使用操作系统原生标题栏；应用顶栏不重复创建这三个控件（用户确认，`EXACT`）。
- 页面内容由 `QGridLayout` / `QBoxLayout` / `QStackedWidget` 组织，不使用截图绝对坐标（`INFERRED`）。
- 调试页维持等宽等高 2×2 图表；较小宿主应滚动，不删除图表（`INFERRED`）。
- 摄像页约为预览 5、控制 3 的列伸缩比（`ESTIMATED`）。
- 参数抽屉和位移面板覆盖底层页面且非模态（`INFERRED`）。

## Components

- `ScientificChart`：标题、轴名、单位、网格、数据状态和曲线。模板只提供稳定模拟曲线，正式工程应接项目图表组件或模型。
- `ParameterDrawer`：11 项真实输入控件；保存/恢复只发 UI 意图。
- `PeakRecordTable`：模板用 `QTableWidget` 展示稳定样例；正式工程宜接共享 `PeakRecord` model。
- `CameraPreview`：模板自绘模拟画面；进入页面不会申请摄像头。
- `MotionZoomPanel`：仅表达显示位移/缩放，不表示移动物理镜头。

## Typography

候选 fallback 为 `Microsoft YaHei UI` / `Segoe UI`（`INFERRED`）；样图的精确字体和字重为 `UNKNOWN`。正式工程字体配置优先。

## Colors & Effects

主操作蓝约 `#087CF0`、深蓝导航约 `#0755A9`、画布约 `#EDF7FF`、白色表面和浅蓝边框（均为 `ESTIMATED`）。面板使用轻边框和约 11 px 候选圆角，不依赖重阴影。

## Spacing & Sizing

重复间距聚类在 6/10/12/16 个候选 Qt 逻辑像素（`INFERRED`）。模板顶栏 64、侧栏 88、页签栏 64 是可回退实现候选（`ESTIMATED`），不是从图片证明的正式逻辑尺寸。

## Responsive Behavior

只提供桌面样图。模板候选窗口为 1504×847、最小 1080×680（`INFERRED`）；真实断点、窄屏重排和多屏 DPI 行为均为 `UNKNOWN`。

## Implementation Guidance

- 页面切换不得自行停止采集、清空数据或重连设备。
- UI 构造函数不得连接 NI、摄像头、网络或执行阻塞 I/O。
- 调试峰值历史与峰值数据页复用同一 `PeakRecord` 数据源；不同 run 不连线，无效样本断线。
- 未校准浓度显示 `— / 未校准`，不把样图数值当真实结果。
- 权限必须由可信业务层执行，隐藏入口不是授权机制。
- 图标和品牌资产应在正式工程确认许可与高 DPI 路径。

## Confidence & Ambiguities

精确项仅包括文件、哈希、画布尺寸和可见结构。语义组件与 Qt 映射为推断，颜色/间距/比例为估计。字体、DPR、客户区、设备、算法、权限、校准和隐藏状态均未由样图证明。

## Designer Confirmation

1. 峰值图缩放究竟采用“两点区间 + 滚轮切换轴 + 右键复位”，还是允许自由拖拽/滚轮缩放。
2. 目标客户区、Windows 缩放、DPR、字体和最小支持尺寸。
3. 正式品牌 Logo、图标、相机画面及样图素材的分发许可。
