# TDLAS 客户 Codex 编译模板

本目录把 `客户` 下 3 张 UI 样图转换为 Codex 可直接读取、修改、构建和截图的 Qt Widgets 模板。原始 PNG 未被覆盖；`resources/reference/` 保存哈希一致的参考副本，且不会作为整页背景编入运行时资源。

## 已实现

- Qt 5/6 Widgets、C++17、CMake、MSVC/GCC/Clang 警告配置。
- 共享顶栏、左侧操作栏和客户可见的两个顶部页签：主页面、摄像界面。
- 客户主页面、11 项参数抽屉、摄像控制和非模态位移/缩放浮层。
- 真实 `QPushButton`、组合框、数值框、滑块和布局；自绘模拟曲线与模拟摄像画面。
- ARX `design-system.json/.md`、Qt `UI_SPEC.md`、令牌、差异日志、参考清单和坐标映射。

## 安全与业务边界

- 所有数据均显式标“模拟”；设备标“未连接”；浓度标“未校准”。
- 不连接 NI、AO/AI、摄像头、网络、登录或权限服务；不读写真实配置、CSV、照片或视频。
- 按钮只表达 UI 意图并显示提示，不伪造设备、采集、拍照、录像或保存成功。
- 页面切换没有采集、清空或重连副作用；隐藏管理员页签不构成权限控制。
- 样图画布不是 Qt 逻辑像素、客户区、DPR 或正式断点。

## 构建

候选验证环境：Qt 6.8.3 `msvc2022_64`、Visual Studio 2022 Build Tools、Release。

```powershell
Set-Location 'E:\AI\氢气3.0\UI设计\客户\codex-qt-template'
.\build.ps1 -QtRoot 'C:\Qt\6.8.3\msvc2022_64'
```

默认输出：`build-client\Release\tdlas_client_codex_template.exe`。首次构建会执行 CMake 配置；后续直接增量编译。

## 运行与截图

```powershell
$env:PATH = 'C:\Qt\6.8.3\msvc2022_64\bin;' + $env:PATH
.\build-client\Release\tdlas_client_codex_template.exe --screen main
```

`--screen` 支持 `main`、`parameters`、`camera`、`camera-motion`；F1～F4 可切换，Esc 先关闭覆盖层，再关闭窗口。

```powershell
Start-Process `
  -FilePath '.\build-client\Release\tdlas_client_codex_template.exe' `
  -ArgumentList @('--screen','camera-motion','--capture','artifacts\captures\camera-motion.png') `
  -WindowStyle Hidden -Wait
```

生成 app-local 部署目录：

```powershell
.\deploy.ps1 -QtRoot 'C:\Qt\6.8.3\msvc2022_64'
.\dist\tdlas_client_codex_template.exe --screen camera
```

## Codex 接入顺序

1. 读取 `docs/ui-specs/design-system/design-system.json` 与 `.md`。
2. 读取 `docs/ui/UI_SPEC.md`、`DESIGN_TOKENS.md`、`DISCREPANCY_LOG.md`。
3. 读取 `docs/ui/reference/MANIFEST.md` 和 `ui-map.json`。
4. 用正式工程的 Qt、主题、组件、数据模型、会话和用例替换 TBD；保留 UI/业务/设备边界。
5. 在已确认的客户区、DPI、字体、主题、语言和稳定测试状态下，对照 C-01～C-03 截图验收。

正式接入前必须确认的问题见 `UI_SPEC.md` 第 8 节。本模板的独立构建和结构截图不代表真实采集、真实摄像、真实算法、客户权限或像素级验收通过。
