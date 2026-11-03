# TDLAS 客户工作台 UI System Specification

## Source

- 模式：`system`。
- 参考：C-01 客户主页面、C-02 客户摄像默认态、C-03 客户摄像位移/缩放浮层态。
- 画布尺寸与 SHA-256 是 `EXACT`；颜色、间距和比例来自栅格观察，标为 `ESTIMATED`；Qt 组件与布局策略标为 `INFERRED`。
- PNG 画布不等同于 Qt 客户区、逻辑像素、DPR 或 Windows 缩放。

## Summary

三张样图共同定义浅蓝白工作台：顶部状态栏、左侧固定操作栏、客户可见的“主页面/摄像界面”两页签和可伸缩内容区。主页面是 2×2 信息布局；摄像页是大预览与右侧控制列；C-03 增加不触发底层重排的位移/缩放浮层。

样图显示“已连接、采集中、浓度数值”，但没有提供设备、算法或标定契约。编译模板因此使用“未连接、就绪、模拟、未校准”，避免把视觉稿伪装成测量结论。

## Component Tree

```text
ClientTemplateWindow
├── TopStatusBar
└── Workspace
    ├── SideActionRail
    └── ContentColumn
        ├── ClientPageTabBar
        └── PageStack
            ├── ClientMainPage
            │   ├── CameraPreviewPanel
            │   ├── FvChartPanel
            │   ├── PeakTrendPanel
            │   ├── RealtimeResultsPanel
            │   └── AcquisitionParameterDrawer (overlay, written-contract evidence)
            └── ClientCameraPage
                ├── PreviewAndCaptureColumn
                ├── CameraControlColumn
                └── MotionZoomPanel (overlay)
```

## Layout

- 顶栏、侧栏和页签条持续可见；页面切换不得停止采集、清空数据或重连设备（`INFERRED`，由设计说明支持）。
- 主页面使用两列两行网格，左上摄像、右上 FV、左下峰值趋势、右下实时结果。模板列 stretch 为 5:6、行 stretch 为 6:5，是可回退候选。
- 摄像页主列与控制列约 5:3（`ESTIMATED`）。左列为预览与拍照/录像控制，右列为设备格式、参数和媒体管理。
- 位移/缩放面板是非模态覆盖层，只改变显示画布，不代表物理镜头运动。
- 目标断点 `UNKNOWN`。模板最小客户区 1080×680 仅为结构候选；正式验收需在目标 DPI、字体和客户区重新校准。

## Reusable Components

| Component | Qt candidate | Responsibility | Confidence |
|---|---|---|---|
| `TopStatusBar` | `QFrame + QHBoxLayout` | 品牌、设备/采集状态、模式和应用动作；窗口控制由 Windows 原生标题栏提供 | INFERRED |
| `ClientPageStack` | `QStackedWidget` | 切换客户主页面和摄像页面 | INFERRED |
| `ScientificChart` | 自绘 `QWidget` 或正式图表组件 | 标题、坐标、单位、曲线和数据状态 | INFERRED |
| `CameraPreview` | 自绘/视频表面 | 主机提供画面与独立摄像状态 | INFERRED |
| `AcquisitionParameterDrawer` | 覆盖式 `QFrame + QFormLayout` | 11 项参数与保存意图 | INFERRED |
| `MotionZoomPanel` | 非模态覆盖式 `QFrame` | 显示位移和缩放意图 | INFERRED |

## Tokens

- 主操作蓝：`#087CF0`（`ESTIMATED`）。
- 深蓝侧栏：`#0755A9`（`ESTIMATED`）。
- 画布：`#EDF7FF`，表面：`#FFFFFF`，边框：`#DBEAF6`（均 `ESTIMATED`）。
- 候选间距级：6 / 10 / 12 / 16 Qt logical px（`INFERRED`）。
- 候选圆角：控件 7、卡片 11、浮层 14 Qt logical px（`ESTIMATED`）。
- 字体精确家族 `UNKNOWN`；模板仅用 Microsoft YaHei UI / Segoe UI fallback。

## Inconsistencies and Boundaries

- C-01～C-03 的实时/连接状态没有业务证据；模板强制标注模拟与未校准。
- 客户只显示两页是可见事实，不是权限契约；正式授权必须由可信层执行。
- 11 项参数抽屉来自书面设计说明而非三张客户样图，不作为像素级参考态。
- 品牌 Logo、正式 SVG 图标和样图中设备照片缺少独立授权资产；模板用文本和自绘模拟画面。

## Implementation Guidance

使用 Qt Layout、stretch 和 SizePolicy，不按截图逐控件绝对定位。整页 PNG 只作为证据，不进入运行时 QRC。UI 构造函数不得连接 NI、AI/AO 或摄像头，不执行阻塞 I/O；按钮只向正式应用层发出意图。浓度、光强、峰值和设备状态必须携带有效性/来源，未校准浓度显示“未校准”或“—”。

## Confidence and Ambiguities

- `EXACT`：三张文件、画布像素、哈希、可见页面/区域/文案。
- `INFERRED`：语义树、布局方式、可复用组件、覆盖层策略和工程边界。
- `ESTIMATED`：颜色、间距、圆角、比例和逻辑尺寸候选。
- `UNKNOWN`：正式客户区、DPR、字体、断点、权限、设备、算法、单位、标定和隐藏状态。
