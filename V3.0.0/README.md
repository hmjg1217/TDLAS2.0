# TDLAS Hydrogen UI Suite V3.0.0

本目录把启动/登录、管理员端和客户端三个 Qt Widgets 样板整理为一个可配置、可构建、可部署的 CMake 工程。主程序 tdlas_hydrogen_ui.exe 在同一进程内完成登录和角色界面切换；三个原样板目标继续保留，只用于开发预览与截图回归。

| 目标 | 可执行文件 | 用途 |
|---|---|---|
| 整合主程序 | tdlas_hydrogen_ui.exe | 登录后进入管理员或客户界面 |
| 启动/登录预览 | tdlas_startup_codex_template.exe | splash、login |
| 管理员端预览 | tdlas_admin_codex_template.exe | main、parameters、debug、peak、camera、camera-motion |
| 客户端预览 | tdlas_client_codex_template.exe | main、parameters、camera、camera-motion |

## 临时登录

| 角色 | 账号 | 密码 |
|---|---|---|
| 管理员 | 00 | 00 |
| 用户 | 01 | 01 |

其他组合统一提示“账号或密码错误”，并清空密码框。固定映射只存在于 apps/integrated/src/TemporaryCredentialAuthenticator.cpp；后期接入正式底层认证时替换该实现即可，登录界面与角色窗口不需要再硬编码账号。

## 目录

~~~text
apps/integrated/              整合主程序、临时认证、窗口路由与测试
apps/startup|admin|client/    三个原样板预览目标及其原规格
shared/resources/             管理员端与客户端可复用的侧栏 PNG/QRC
docs/ui/                      本次整合规格、清单和差异记录
tools/capture-all.ps1         生成 13 个页面/状态候选截图
~~~

原工程中的 build*、dist、CMake cache 和 VS 自动生成文件没有复制；这些文件带有旧工程绝对路径，必须在本目录重新生成。

## 构建

~~~powershell
Set-Location 'C:\Users\91463\Documents\ChatGPT\N.UI\TDLAS-Hydrogen-UI-Suite'
.\build.ps1 -QtRoot 'C:\Qt\6.8.3\msvc2022_64'
~~~

默认输出到 build\bin\Release。只构建主程序时可追加 -Target integrated；也可使用 -Target startup、-Target admin 或 -Target client。

测试：

~~~powershell
& 'C:\Qt\Tools\CMake_64\bin\ctest.exe' --test-dir .\build -C Release --output-on-failure
~~~

测试覆盖固定凭据映射、错误凭据停留登录页、密码清空、管理员/客户窗口类型与对应 QSS 切换。

## 部署

~~~powershell
.\deploy.ps1 -QtRoot 'C:\Qt\6.8.3\msvc2022_64'
~~~

默认 dist 只包含整合主程序及其 Qt/MSVC 运行库，防止直接启动管理员预览程序绕过登录。确需开发预览包时使用：

~~~powershell
.\deploy.ps1 -QtRoot 'C:\Qt\6.8.3\msvc2022_64' -IncludePreviewTargets
~~~

## 截图验收

~~~powershell
.\tools\capture-all.ps1
~~~

候选截图写入 artifacts\captures。每个样板仍支持单独运行，例如：

~~~powershell
.\build\bin\Release\tdlas_client_codex_template.exe --screen camera --capture .\artifacts\captures\client-camera.png
~~~

## 共享侧栏控件

整合主程序、管理员预览和客户端预览均已编译 shared/resources/sidebar-icons.qrc，但现有页面尚未替换原控件。后续可直接使用以下资源地址：

| 用途 | Qt 资源地址 |
|---|---|
| 主页面 | :/sidebar-icons/home.png |
| 参数区 | :/sidebar-icons/parameters.png |
| 开始采集 | :/sidebar-icons/start-acquisition.png |
| 停止采集 | :/sidebar-icons/stop-acquisition.png |
| 数据保存 | :/sidebar-icons/save-data.png |

这些 PNG 来自用户指定目录，均为 1254×1254 ARGB。发布许可与最终显示尺寸仍需在正式产品接入时确认。

## 集成边界

管理员和客户端类分别放入 adminui 与 clientui 命名空间，消除了单进程同名符号冲突。切换角色时只加载当前角色 QSS，避免宽泛全局选择器互相污染。

本轮只实现用户指定的临时本地账号路由，没有接入数据库、网络、权限服务、设备、算法、摄像头或真实数据保存。管理员界面可见性仍不等同于正式权限校验；发布前应把 TemporaryCredentialAuthenticator 替换为可信认证与授权层。
