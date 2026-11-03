# TDLAS 管理员 Codex 编译模板

本目录把 `管理员` 下 6 张 UI 样图转换为 Codex 可直接读取、修改、构建和截图的 Qt Widgets 模板。原始 PNG 未被覆盖；`resources/reference/` 是哈希一致的参考副本，且不会作为整页背景编入运行时资源。

## 已实现

- Qt 5/6 Widgets、C++17、CMake、MSVC/GCC/Clang 警告配置。
- 共享顶栏、侧栏、四页导航和六种预览状态。
- 主页面、11 项参数抽屉、调试 2×2 图表、峰值筛选/趋势/表格、摄像控制和位移/缩放浮层。
- 真实 `QPushButton`、输入、组合框、滑块、表格和布局；自绘模拟曲线与模拟摄像画面。
- ARX `design-system.json/.md`、Qt `UI_SPEC.md`、令牌、差异日志、参考清单和坐标映射。

## 安全与业务边界

- 所有数据均显式标“模拟”；设备标“未连接”；浓度标“未校准”。
- 不连接 NI、AO/AI、摄像头、网络或登录/权限；不读写真实配置、CSV、照片或视频。
- 按钮只发 UI 意图并显示提示，不伪造设备或业务成功。
- 页面切换没有采集、清空或重连副作用。
- 样图画布不是 Qt 逻辑像素、客户区、DPR 或正式断点。

## 构建

本机已验证：Qt 6.8.3 `msvc2022_64`、Visual Studio 2022 Build Tools、Release。

```powershell
Set-Location 'E:\AI\氢气3.0\UI设计\管理员\codex-qt-template'
.\build.ps1 -QtRoot 'C:\Qt\6.8.3\msvc2022_64'
```

默认输出：`build-admin\Release\tdlas_admin_codex_template.exe`。首次构建会执行 CMake 配置；后续直接增量编译。若要强制使用新的生成目录，传入 `-BuildDirectory 'build-fresh'`。

## 运行与截图

```powershell
$env:PATH = 'C:\Qt\6.8.3\msvc2022_64\bin;' + $env:PATH
.\build-admin\Release\tdlas_admin_codex_template.exe --screen main
```

`--screen` 支持：

```text
main
parameters
debug
peak
camera
camera-motion
```

也可在运行时用 F1～F6 切换；Esc 先关闭浮层，再关闭窗口。

截图示例：

```powershell
Start-Process `
  -FilePath '.\build-admin\Release\tdlas_admin_codex_template.exe' `
  -ArgumentList @('--screen','debug','--capture','artifacts\captures\debug.png') `
  -WindowStyle Hidden -Wait
```

生成可独立运行的 Qt 部署目录：

```powershell
.\deploy.ps1 -QtRoot 'C:\Qt\6.8.3\msvc2022_64'
.\dist\tdlas_admin_codex_template.exe --screen camera
```

## Codex 接入顺序

1. 读取 `docs/ui-specs/design-system/design-system.json` 与 `.md`。
2. 读取 `docs/ui/UI_SPEC.md`、`DESIGN_TOKENS.md`、`DISCREPANCY_LOG.md`。
3. 读取 `docs/ui/reference/MANIFEST.md` 和 `ui-map.json`。
4. 用正式工程的 Qt、主题、组件、数据模型和用例替换 TBD；保留 UI/业务/设备边界。
5. 在已确认的客户区、DPI、字体、主题、语言和稳定测试状态下截图对照 A-01～A-06。

正式接入前必须确认的问题见 `UI_SPEC.md` 第 8 节。本模板已完成独立构建和结构级截图检查，但不代表真实采集、真实摄像、真实算法、权限或像素级验收通过。
