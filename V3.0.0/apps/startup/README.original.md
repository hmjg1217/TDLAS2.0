# TDLAS 启动与登录 Codex 编译模板

本目录把两张原始 UI 样图转换为 Codex 可直接读取、修改和编译的 Qt Widgets 模板。原始 PNG 未被覆盖；`resources/reference/` 中是构建所需的只读副本。

## 交付边界

- 启动页复用品牌位图，保持宽高比显示，不伪造加载百分比或真实初始化状态。
- 登录页以原图作为视觉画布，并用真实 `QLineEdit`、密码回显模式和 `QPushButton` 覆盖表单区。
- 模板不保存凭据、不连接网络、不判断角色、不进入业务主界面。
- 登录按钮只发出 `loginSubmitted(username, password)`；正式认证、错误映射、会话与权限由目标工程接管。
- 原图像素是视觉证据，不等于 Qt 逻辑像素、客户区、DPR 或正式断点。

## 目录

```text
codex-qt-template/
├── CMakeLists.txt
├── build.ps1
├── src/                         Qt 5/6 Widgets 编译骨架
├── resources/                   QRC、样式和参考图副本
├── docs/ui/                     Qt 页面、状态、TBD 与验收契约
└── docs/ui-specs/design-system/ ARX 机器/人工可读系统规格
```

## 配置与构建

当前机器已验证的环境候选是 Qt 6.8.3 `msvc2022_64`、Visual Studio 2022 Build Tools 和 Qt Tools CMake。它只是本模板的验证环境，不是产品版本决定。

```powershell
Set-Location 'E:\AI\氢气3.0\UI设计\启动\codex-qt-template'
.\build.ps1 -QtRoot 'C:\Qt\6.8.3\msvc2022_64'
```

也可以手动配置：

```powershell
& 'C:\Qt\Tools\CMake_64\bin\cmake.exe' `
  -S . -B build -G 'Visual Studio 17 2022' -A x64 `
  -DCMAKE_PREFIX_PATH='C:\Qt\6.8.3\msvc2022_64'
& 'C:\Qt\Tools\CMake_64\bin\cmake.exe' --build build --config Release --parallel
```

## 运行与截图

`build\Release` 中的 EXE 是构建产物，不能脱离 Qt SDK 单独运行。双击前应先生成包含 Qt DLL 和平台插件的部署目录：

```powershell
Set-Location 'E:\AI\氢气3.0\UI设计\启动\codex-qt-template'
.\deploy.ps1 -QtRoot 'C:\Qt\6.8.3\msvc2022_64'
.\dist\tdlas_startup_codex_template.exe --screen login
```

请分发整个 `dist` 目录，不能只复制其中的 EXE。部署脚本同时补齐 Qt DLL、`platforms\qwindows.dll` 和 MSVC app-local 运行库。

```powershell
$env:PATH = 'C:\Qt\6.8.3\msvc2022_64\bin;' + $env:PATH
.\build\Release\tdlas_startup_codex_template.exe --screen splash
.\build\Release\tdlas_startup_codex_template.exe --screen login

# Windows QPA 隐藏进程截图；文件写入 artifacts/captures/
Start-Process `
  -FilePath '.\build\Release\tdlas_startup_codex_template.exe' `
  -ArgumentList @('--screen','login','--capture','artifacts\captures\login-candidate.png') `
  -WindowStyle Hidden -Wait
```

`F2` 在启动页与登录页之间切换；`Esc` 关闭预览。`--screen` 只选择开发预览状态，不代表真实加载流程。

本机 Qt `offscreen` 平台会把控件字体渲染为方框，因此只把它作为诊断路径，不作为视觉验收基准；当前有效截图使用 Windows QPA 生成。

## Codex 接入顺序

1. 读取 `docs/ui-specs/design-system/design-system.json` 和 `design-system.md`。
2. 读取 `docs/ui/UI_SPEC.md`、`DESIGN_TOKENS.md`、`reference/MANIFEST.md` 与 `reference/ui-map.json`。
3. 用目标工程的 Qt 版本、入口、主题、资源机制和认证接口替换所有 `TBD`。
4. 复用真实控件和布局；不要把登录整图当作不可交互页面，也不要手改生成的 `ui_*.h`。
5. 在已确认的客户区、DPI、字体、语言和主题下截图，与 Ref S-01/S-02 对照。

详细未知项和关闭条件见 `docs/ui/UI_SPEC.md` 第 8 节。模板目前可用于结构实现和视觉校准，不能据此宣称真实登录或像素级验收完成。
