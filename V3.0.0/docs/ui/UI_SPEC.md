# TDLAS Hydrogen UI Suite 整合规格

## 1. 文档控制与目标

- 项目 / 目标：把启动/登录、管理员端、客户端三个 Qt Widgets 样板整合为一个可构建、可部署的单进程主程序，同时保留独立预览目标。
- 版本 / 日期 / 负责人：0.2.0 Implemented / 2026-11-02 / Codex；产品最终确认待用户。
- 本次模式：工程审计 + 单进程整合 + 临时账号路由 + 构建/测试/部署/截图 QA。
- 包含：整合主程序、三个预览目标、原规格与参考图、原候选截图、公共侧栏 PNG、统一脚本、认证与 GUI 路由测试。
- 用户确认的临时路由：管理员 00/00；用户 01/01；其他组合无效。
- 不包含：正式认证/授权服务、账号管理、密码存储、会话恢复/失效、退出登录、设备、算法、网络、摄像头、真实数据保存或持久化。
- 技术环境：C++17、Qt 5/6 Widgets、CMake 3.21+；本轮实测为 Qt 6.8.3 + MSVC 2022 x64。
- 用户授权来源：2026-11-02 当前请求；指定三个样板目录、侧栏控件目录、新工程输出目录及临时账号。
- AGENTS.md：工作区与四个来源目录均未发现。

证据标记使用 CONFIRMED / INFERRED / TBD / N/A。各页面详细视觉、状态、数据与业务边界以 apps/*/docs/ui 下的原规格为准。

## 2. 输入与环境

| 字段 | 值 | 证据 | 来源 / TBD |
|---|---|---|---|
| 启动样板 | E:\AI\氢气3.0\UI设计\启动\codex-qt-template | CONFIRMED | 用户指定、只读审计 |
| 管理员样板 | E:\AI\氢气3.0\UI设计\管理员\codex-qt-template | CONFIRMED | 用户指定、只读审计 |
| 客户样板 | E:\AI\氢气3.0\UI设计\客户\codex-qt-template | CONFIRMED | 用户指定、只读审计 |
| 侧栏控件 | E:\AI\氢气3.0\UI设计\控件，5 张 1254×1254 ARGB PNG | CONFIRMED | 尺寸、透明度、SHA-256 实测 |
| 整合输出 | C:\Users\91463\Documents\ChatGPT\N.UI\TDLAS-Hydrogen-UI-Suite | CONFIRMED | 当前请求 |
| 验证环境 | Windows 10 SDK 10.0.26100.0、Qt 6.8.3、MSVC 19.44、VS 2022 x64 | CONFIRMED | CMake/MSBuild 输出 |
| 正式 DPI / DPR / 字体 / 客户区 | 继承各样板候选；正式目标未确认 | TBD | TBD-INT-002 |

## 3. 工程审计与不变量

| 项目 | 整合现状 | 证据 | 本次影响 |
|---|---|---|---|
| 主目标 | tdlas_hydrogen_ui | CONFIRMED | 登录后在同一 QApplication 内创建目标角色窗口 |
| 预览目标 | startup、admin、client 三个原 target | CONFIRMED | 仅开发回归；默认不部署 |
| 测试目标 | tdlas_authentication_test、tdlas_login_routing_test | CONFIRMED | CTest 覆盖纯认证与真实 Qt 控件路由 |
| 同名类 | 管理员/客户 ChartWidget、CameraPreviewWidget 实现不同 | CONFIRMED | 分别放入 adminui / clientui 命名空间 |
| QSS | 三份样式含应用级选择器 | CONFIRMED | 登录成功后替换为当前角色 QSS，不拼接 |
| QRC | 主程序编入启动、管理员、客户与共享侧栏资源 | CONFIRMED | 共享图标使用稳定英文 alias |
| 认证 | TemporaryCredentialAuthenticator 中固定两组凭据 | CONFIRMED | UI 不硬编码；后续替换单一底层实现 |
| 原目录 | 只读，未修改 | CONFIRMED | 回退只需删除新工程 |
| 旧产物 | 原 build*、dist 未复制 | CONFIRMED | 新工程重新配置、构建、部署 |

不变量：临时登录只用于当前样板路由，不声明为正式安全认证；页面可见性不替代授权；不得伪造设备连接、采集、摄像、算法结果或数据保存。

## 4. 页面与共享组件

| Page ID | 页面 / 角色 | 主程序入口 | 状态 | 详细规格 |
|---|---|---|---|---|
| PAGE-S-LOGIN | 登录 / 未认证 | 应用默认入口 | idle、validation、auth-error、success-route | apps/startup/docs/ui/UI_SPEC.md |
| PAGE-A-SHELL | 管理员 | 00/00 登录成功 | main、parameters、debug、peak、camera、camera-motion | apps/admin/docs/ui/UI_SPEC.md |
| PAGE-C-SHELL | 客户 | 01/01 登录成功 | main、parameters、camera、camera-motion | apps/client/docs/ui/UI_SPEC.md |
| PAGE-S-STARTUP | 启动展示 / 预览 | startup 预览 target | brand-visible | apps/startup/docs/ui/UI_SPEC.md |

| Component ID | 职责 | 实现 / 路径 | 证据 |
|---|---|---|---|
| CMP-INT-CONTROLLER | 登录窗口、角色路由、QSS 与窗口生命周期 | apps/integrated/src/IntegratedApplicationController.* | CONFIRMED、CTest PASS |
| CMP-INT-AUTH | 临时账号到角色映射 | apps/integrated/src/TemporaryCredentialAuthenticator.* | CONFIRMED、CTest PASS |
| CMP-INT-BUILD | 统一配置与构建 | CMakeLists.txt、build.ps1 | CONFIRMED、Release PASS |
| CMP-INT-DEPLOY | 默认只部署主程序及 app-local 运行库 | deploy.ps1 | CONFIRMED、deploy/run PASS |
| CMP-INT-CAPTURE | 整合登录 + 12 个预览状态截图 | tools/capture-all.ps1 | CONFIRMED、13/13 PASS |
| CMP-INT-SIDEBAR | 后续侧栏图标资源 | shared/resources/sidebar-icons.qrc | CONFIRMED；当前未绑定控件 |

## 5. 整合结构

~~~text
TDLAS-Hydrogen-UI-Suite/
  CMakeLists.txt
  build.ps1
  deploy.ps1
  apps/
    integrated/
    startup/
    admin/
    client/
  shared/resources/sidebar-icons/
  docs/ui/
  artifacts/
    source-baselines/
    captures/
    qa/
  tools/capture-all.ps1
~~~

- apps/integrated 是正式主入口；三个原 main.cpp 只编入各自预览 target。
- 临时账号只存在 TemporaryCredentialAuthenticator.cpp。
- 原中文图标完整保留；compiled/ 是逐字节相同、仅英文命名的 RCC 输入。
- 原候选截图只作追溯；本次截图与 QA 联系表分别位于 artifacts/captures 和 artifacts/qa。

## 6. 导航、角色与状态

| From | Event | Guard | To | 失败行为 | 证据 |
|---|---|---|---|---|---|
| PAGE-S-LOGIN | 提交 00/00 | TemporaryCredentialAuthenticator → Administrator | PAGE-A-SHELL / Main | N/A | 用户确认 + CTest PASS |
| PAGE-S-LOGIN | 提交 01/01 | TemporaryCredentialAuthenticator → Client | PAGE-C-SHELL / Main | N/A | 用户确认 + CTest PASS |
| PAGE-S-LOGIN | 其他非空组合 | Invalid | 留在登录页 | 通用错误；清空密码并恢复焦点 | CTest PASS |
| PAGE-S-LOGIN | 空账号或密码 | 本地字段校验 | 留在登录页 | “请输入账号和密码” | 既有实现 |

成功顺序：禁用登录控件 → 清空密码 → 隐藏登录页 → 切换当前角色 QSS → 创建并显示角色窗口 → 关闭并销毁登录页。关闭角色窗口后退出应用。用户名沿用既有 trimmed() 处理；密码不裁剪空格。

## 7. 设计令牌与资源

- 各样板原 QSS 仍是角色视觉事实源；不创建冲突的跨角色 token。
- QApplication 同一时刻只加载 startup.qss、admin.qss 或 client.qss 之一。
- 公共侧栏资源提供 :/sidebar-icons/* alias，没有擅自决定 iconSize、状态变体或按钮替换范围。
- 详见 docs/ui/DESIGN_TOKENS.md 与 docs/ui/reference/MANIFEST.md。

## 8. TBD 与决定记录

| ID | 问题 / 决定 | 依据 | 影响 | 状态 |
|---|---|---|---|---|
| TBD-INT-001 | 单进程与临时角色路由 | 用户于 2026-11-02 指定 00/00 与 01/01 | 本轮主程序 | Closed / 已实施验证 |
| TBD-INT-002 | 正式 Qt、编译器、DPI/DPR、字体与客户区 | 只有本机 Qt 6.8.3 候选 | 像素级验收与发布 | Open |
| TBD-INT-003 | 样图、品牌与侧栏 PNG 的正式分发许可 | 用户提供并授权本轮内部整合 | 对外发布 | Open |
| TBD-INT-004 | 五张侧栏 PNG 的按钮映射、尺寸和状态图 | 用户说明后期需要时调用 | 实际替换控件 | Open；仅编译资源 |
| TBD-INT-005 | 正式认证、授权、账号存储、会话失效与退出登录 | 当前仅临时明文映射 | 生产安全与完整工作流 | Open；发布前必须替换 |

## 9. 工作包与修改边界

| Work Package | 目标 | allowlist | 不变量 / denylist | 验证 |
|---|---|---|---|---|
| WP-INT-01 | 复制三个样板与证据 | 新文件夹 apps、artifacts/source-baselines | 不修改原目录；不复制 build/dist | 初始 58/58 文件哈希一致 |
| WP-INT-02 | 根工程与预览 target | 根 CMake、脚本、apps/*/CMakeLists | 不改变页面结构/业务语义 | 全目标 Release build |
| WP-INT-03 | 单进程与角色路由 | apps/integrated、角色命名空间、登录清密接口 | 不实现正式权限或存储 | 2 项 CTest |
| WP-INT-04 | 共享侧栏资源 | shared/resources、相关 CMake | 不替换现有按钮 | RCC build、派生哈希 |
| WP-INT-05 | 部署与 QA | deploy.ps1、tools、artifacts/captures、docs | 默认包不含预览 EXE | deploy/run、13 截图、12 状态哈希 |

回退方式：删除 C:\Users\91463\Documents\ChatGPT\N.UI\TDLAS-Hydrogen-UI-Suite；三个原样板不受影响。

## 10. 验证与截图矩阵

| VER ID | 类型 | 命令 / 操作 | 实际结果 |
|---|---|---|---|
| VER-INT-001 | configure/build | .\build.ps1 -QtRoot C:\Qt\6.8.3\msvc2022_64 -Target all | PASS；主程序、3 预览、2 测试 EXE 生成 |
| VER-INT-002 | unit/GUI | ctest --test-dir build -C Release --output-on-failure | PASS；2/2，0.34 s |
| VER-INT-003 | render | .\tools\capture-all.ps1 | PASS；13/13 exit 0、PNG 非空 |
| VER-INT-004 | source parity | 原目录当前 dist EXE 现场捕获 12 个状态并与整合预览比较 SHA-256 | PASS；12/12 完全一致 |
| VER-INT-005 | visual | 查看 artifacts/qa/capture-contact-sheet.png | PASS（结构级）；无缺失、拉伸或主要溢出 |
| VER-INT-006 | deploy | .\deploy.ps1 | PASS；默认仅 tdlas_hydrogen_ui.exe，Qt/MSVC 必需文件齐全，55.88 MiB |
| VER-INT-007 | app-local run | 清除 Qt SDK PATH 后运行 dist 主程序 --capture | PASS；exit 0，截图与 build 版哈希一致 |

构建期间唯一工具提示为可选 Vulkan headers 未安装；本工程只链接 Qt Widgets，不使用 Vulkan。windeployqt 未找到可选 dxcompiler/dxil，不影响本轮 app-local 启动验证。

候选截图：artifacts/captures 下 14 张（13 状态 + 部署登录复验）；均为本轮实际运行输出。整合登录图与独立登录图哈希一致。

正式设备、算法、摄像、数据、授权和像素级目标环境回归未运行，因为相关接口与校准环境不在本轮范围。

## 11. 差异、确认与交付

- 差异日志：docs/ui/DISCREPANCY_LOG.md。
- 本次结构决定：一个正式单进程主程序 + 三个不随默认包发布的开发预览目标。
- 已关闭：同名 C++ 类冲突、三 main 冲突、跨角色 QSS 污染风险、旧 CMake cache 搬运风险、测试 Qt DLL 路径。
- allowlist 外变化：无；原 E:\AI\氢气3.0 目录未修改。
- 验证结论：本轮工程整合、临时登录路由、构建、测试、部署和结构级视觉回归通过。
- 剩余限制：TBD-INT-002～005；尤其临时凭据不是生产认证。
