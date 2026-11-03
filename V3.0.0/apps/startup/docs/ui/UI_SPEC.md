# TDLAS 启动与登录 Qt UI 技术规格

> 整合备注（2026-11-02）：页面实现保持不变；整合主程序由 IntegratedApplicationController 接收 loginSubmitted。临时路由为 00/00 → 管理员、01/01 → 用户，映射位于 apps/integrated/src/TemporaryCredentialAuthenticator.cpp。独立 startup target 仍只作预览。

## 1 文档控制与目标

- 项目 / 目标：把启动批次两张栅格样图转换为 Codex 可读取、可编译、可继续集成的 Qt Widgets 模板。
- 版本 / 日期 / 负责人：0.1.0 Draft / 2026-11-02 / Codex 生成，产品与 UX 待确认。
- 状态：Draft；未进入正式目标工程验收。
- 本次模式：图片转规格 + 可回退编译模板。
- 包含：启动品牌展示页、登录 idle/本地校验/提交意图、资源、QSS、构建与截图命令。
- 不包含：真实启动进度、认证服务、凭据存储、角色判断、主界面导航、硬件、网络、部署。
- 技术候选：C++17、Qt 5/6 Widgets、CMake；本机以 Qt 6.8.3 MSVC 2022 验证。
- 用户授权来源：2026-11-02 当前请求，允许转换 `启动` 目录样图为 Codex 编译模板。
- 适用 AGENTS.md：目标目录及上级未发现 `AGENTS.md`；以当前请求和 `CODEX_UI_WORKFLOW.md` 边界为准。

证据使用 CONFIRMED / INFERRED / TBD / N/A。图像中的可见对象可确认，Qt 类型、逻辑尺寸、状态机和业务语义不能只凭图片确认。

## 2 输入与环境

| 字段 | 值 | 证据状态 | 来源 / TBD ID |
|---|---|---|---|
| Reference Manifest | `docs/ui/reference/MANIFEST.md` | CONFIRMED | 本模板 |
| 参考图 | S-01 1774×887 ARGB；S-02 1671×941 RGB | CONFIRMED | PNG 元数据与 SHA-256 |
| 目标客户区逻辑尺寸 | 未给出；模板候选最小 960×540 | TBD / INFERRED | TBD-003 |
| OS / Qt / 平台样式 | 正式目标 TBD；验证为 Windows + Qt 6.8.3 + Windows QPA | CONFIRMED（验证环境） | 构建/截图记录 |
| DPI / DPR / OS 缩放 | 未记录；不能从 PNG 96 DPI 标签推导 | TBD | TBD-003 |
| 字体 / 主题 / 语言 | 浅色；英文占位；字体 fallback 候选 | INFERRED / TBD | TBD-003、TBD-008 |
| 稳定数据 / 截图 | 空账号/密码；`QWidget::grab()` | CONFIRMED（本轮） | CAP-001、CAP-002 |

## 3 工程审计与不变量

| 项目 | 现状 / 实际路径 / 符号 | 证据 | 本次影响 |
|---|---|---|---|
| 构建系统与目标 | `CMakeLists.txt`；target `tdlas_startup_codex_template` | CONFIRMED | 新建独立模板，不引用 `TDLAS_UI_Template` 源码 |
| 页面入口 | `src/main.cpp`；`--screen splash|login` | CONFIRMED | 仅开发预览选择，不代表真实导航 |
| UI / QSS / 资源 | `StartupTemplateWindow.*`、`startup.qss`、`startup.qrc` | CONFIRMED | 真实控件覆盖登录表单 |
| 共享组件 | `StartupTemplateWindow`、`ReferenceCanvas` paint path | CONFIRMED | 目标工程可拆分为 Splash/Login 页面 |
| 业务 / 权限 / 网络 / 硬件 / 持久化 | 未接入 | CONFIRMED | 必须保持未接入，直至目标接口授权 |
| 用户已有修改 | 只有原始 PNG；未覆盖 | CONFIRMED | 资源副本保持 SHA-256 一致 |

业务不变量：UI 只收集输入并发出登录意图；不存凭据、不泄露账号存在性、不自行判断角色、不连接设备或网络。图片不证明加载进度、启动耗时、自动跳转、记住我、密码可见性或错误映射。

## 4 页面与共享组件

| Page ID | 页面 / 角色 | 入口 | 实际类 / 文件 | Ref | 适用状态 | 优先级 | 证据 / 规格状态 |
|---|---|---|---|---|---|---|---|
| PAGE-STARTUP | 启动品牌展示 / 未认证 | `--screen splash` 或宿主选择 | `StartupTemplateWindow` / `StartupTemplateWindow.cpp` | S-01 | brand-visible | P0 | 视觉 CONFIRMED；生命周期 TBD |
| PAGE-LOGIN | 登录 / 未认证 | `--screen login` 或宿主选择 | 同上 | S-02 | idle、validation-error、submit-intent；其余宿主控制 | P0 | idle 视觉 CONFIRMED；业务 TBD |

| Region / Component ID | 职责 | 页面 | Qt 类型 | 输入 / 输出 | 证据 |
|---|---|---|---|---|---|
| CMP-REFERENCE-CANVAS | 保持比例绘制参考画布 | 两页 | `QWidget::paintEvent` | QRC pixmap → rendered canvas | INFERRED |
| CMP-USERNAME | 收集账号 | PAGE-LOGIN | `QLineEdit` | 文本 → login intent | 可见字段 CONFIRMED；类型 INFERRED |
| CMP-PASSWORD | 收集并遮罩密码 | PAGE-LOGIN | `QLineEdit::Password` | 密码 → login intent | 字段 CONFIRMED；安全类型 INFERRED |
| CMP-LOGIN | 提交登录意图 | PAGE-LOGIN | `QPushButton` | click/Enter → `loginSubmitted` | 按钮 CONFIRMED；业务 TBD |
| CMP-STATUS | 表达本地校验/宿主结果 | PAGE-LOGIN | `QLabel` | status text | INFERRED；样图未展示错误态 |

## 5 页面规格

### 5.1 PAGE-STARTUP

- 职责：展示品牌、产品名、描述、科学插画和出品信息。
- 可见事实：单一品牌卡片；没有可见进度条、百分比、取消按钮或错误提示。
- 根容器：`StartupTemplateWindow`，`Screen::Splash`。
- 区域树：`StartupPage → BrandCanvas → BrandMark / ProductTitle / ProductDescriptor / Artwork / Attribution`。

| Region | Qt 容器 / Layout | 对齐与 SizePolicy | resize / 溢出 | 证据 |
|---|---|---|---|---|
| REG-STARTUP-CANVAS | 自绘 `QWidget` | centered / expanding | `KeepAspectRatio`；letterbox；不裁剪 | INFERRED |

启动页不实现计时或自动跳转。真实初始化状态、最短展示时间、失败态和目标页由 TBD-005 关闭后接入。

### 5.2 PAGE-LOGIN

- 职责：展示品牌登录页，采集账号和密码，发出提交意图。
- 可见事实：左侧科学插画；右侧品牌、产品标题、账号、密码和 Login 按钮。
- 默认焦点：账号框（实现候选）。
- 区域树：`LoginPage → ReferenceCanvas → IllustrationRegion + AuthenticationRegion → BrandHeader + CredentialForm`。

| Region | Qt 容器 / Layout | margins / spacing | stretch / 对齐 | SizePolicy | resize / 溢出 | 证据 / TBD |
|---|---|---|---|---|---|---|
| REG-LOGIN-CANVAS | 自绘 `QWidget` | N/A | centered | expanding | 保持 S-02 比例，letterbox | INFERRED |
| REG-CREDENTIAL-FORM | `QFrame + QVBoxLayout` | 0；18/34 px candidate | 纵向 | relative fixed-composition region | 相对 fitted image rect 定位 | ESTIMATED；TBD-003 |

| Component ID | 语义 | Qt 类型 / objectName | 文本 | 输出意图 | 状态 | 无障碍 | 证据 |
|---|---|---|---|---|---|---|---|
| CMP-USERNAME | 账号 | `QLineEdit#usernameEdit` | `Username` | username | normal/focus/disabled/error-via-message | accessibleName `账号` | 可见 CONFIRMED；实现 INFERRED |
| CMP-PASSWORD | 密码 | `QLineEdit#passwordEdit` | `Password` | password | masked/focus/disabled/error-via-message | accessibleName `密码` | 可见 CONFIRMED；echo mode INFERRED |
| CMP-LOGIN | 登录 | `QPushButton#loginButton` | `Login` | `loginSubmitted` | normal/hover/pressed/focus/disabled | accessibleName `登录` | 可见 CONFIRMED；状态候选 INFERRED |
| CMP-STATUS | 反馈 | `QLabel#loginMessage` | 动态 | none | info/error | accessibleName `登录状态` | INFERRED |

### 5.3 状态模型

| State ID | 进入条件 | visible / enabled | 文案 / 数据 | 动作 | 退出条件 | 错误规则 | 证据 |
|---|---|---|---|---|---|---|---|
| LOGIN-IDLE | 页面进入 | 表单可用 | 空输入 | 输入/提交 | submit 或离开 | N/A | S-02 CONFIRMED |
| LOGIN-VALIDATION-ERROR | 任一字段为空 | 表单可用 | `请输入账号和密码。` | 修正/重试 | 两字段有效 | 不泄露账号存在性 | INFERRED，实现候选 |
| LOGIN-SUBMIT-INTENT | 本地字段非空并提交 | 由宿主决定 busy | 只发信号 | 宿主接管 | 成功/失败/取消 | 模板默认返回“接口未配置” | INFERRED |
| LOGIN-BUSY | 宿主开始认证 | 表单与按钮禁用 | `Signing in…` candidate | 不允许重复提交 | 宿主完成 | 超时/取消由宿主定义 | TBD-006 |
| LOGIN-AUTH-ERROR | 宿主认证失败 | 表单恢复 | 通用错误，具体映射沿用安全策略 | 重试 | 新提交/离开 | 不区分“账号不存在/密码错误”除非契约允许 | TBD-006 |
| LOGIN-SUCCESS | 宿主认证成功 | 由导航层决定 | N/A | 导航 | session ends | UI 不自行判断角色 | TBD-006、TBD-007 |

### 5.4 事件与动作

| Event ID | 触发源 / 键盘 | 前置条件 | 即时反馈 | 信号 / 用例 | 成功 | 失败 | 防重复 | 证据 |
|---|---|---|---|---|---|---|---|---|
| EVT-LOGIN-SUBMIT | 按钮 click / 密码框 Enter | 本地非空 | 状态文案 | `loginSubmitted(username,password)` | 宿主处理 | 本地错误或宿主错误 | `setLoginBusy(true)` 候选 | INFERRED |
| EVT-PREVIEW-TOGGLE | F2 | 开发模板 | 切换 S-01/S-02 | 本地预览 | N/A | N/A | N/A | N/A，仅开发工具 |
| EVT-CLOSE | Esc | 开发模板 | 关闭 | QWidget close | N/A | N/A | N/A | N/A，仅开发工具 |

### 5.5 键盘和无障碍

- Tab：账号 → 密码 → 登录按钮。
- Enter：密码框或默认按钮提交。
- Escape：只在独立预览模板中关闭；集成后沿用目标工程窗口策略。
- 焦点：QSS 提供非颜色以外的边框/下划线变化；错误使用文字，不只使用颜色。
- 密码可见性按钮和“记住我”未出现在样图中，不擅自添加。

## 6 导航与角色

| From | Event | Guard / 来源 | To | 返回 / 会话失效 | 不允许情况 | 证据 |
|---|---|---|---|---|---|---|
| PAGE-STARTUP | 初始化完成 | 目标工程启动协调器 | PAGE-LOGIN 或会话恢复目标 | TBD | UI 自行用计时器假装完成 | TBD-005 |
| PAGE-LOGIN | auth success | 可信认证/会话层 | 目标主页 | 会话失效按既有契约 | 仅隐藏按钮充当权限、UI 猜角色 | TBD-006、TBD-007 |

## 7 设计令牌

- 候选事实源：`docs/ui/DESIGN_TOKENS.md` 与 `resources/styles/startup.qss`。
- 正式工程存在主题系统时，以其为唯一事实源并映射本规格，不复制硬编码。
- Logo 与插画不建议用 QSS 重绘；使用经许可的批准资产。

## 8 TBD 与决定记录

| TBD ID | 位置 / 字段 | 问题 | 当前证据 | 所需证据 / 决定 | Owner | 阻塞 | 临时策略 | 状态 |
|---|---|---|---|---|---|---|---|---|
| TBD-001 | 工程集成 | 正式目标工程绝对路径和入口未提供 | 当前只有独立模板 | 目标 repo、入口、AGENTS、工作树 | User / Tech Lead | 正式集成 | 保持独立可删除目录 | Open |
| TBD-002 | 工具链 | 正式 Qt/编译器/构建系统未决定 | 本机 Qt 6.8.3 构建通过 | 产品支持矩阵或目标工程配置 | Tech Lead | 正式构建 | CMake 同时支持 Qt 5/6 | Open |
| TBD-003 | 校准 | 客户区、窗口边框、DPI/DPR、缩放、字体未确认 | 只有栅格尺寸 | 统一截图环境 | UX / QA | 像素级验收 | 相对比例 + 960×540 可回退候选 | Open |
| TBD-004 | 资产 | 正式分发许可和源 Logo 未提供 | 用户提供样图 | 资产权利与原始品牌资源 | Product | 发布 | 仅内部模板使用同哈希副本 | Open |
| TBD-005 | 启动流程 | 初始化状态、时长、错误和跳转未知 | S-01 只有品牌展示 | 启动协调器契约 | App Lead | 启动行为 | 无计时/无进度；预览手动选状态 | Open |
| TBD-006 | 认证 | 接口、busy、超时、错误映射、会话未知 | S-02 只有表单 | 认证用例/接口 | Security / App Lead | 真实登录 | 只发登录意图，不执行认证 | Open |
| TBD-007 | 角色 | 登录后角色与目标页未知 | 图片无证据 | 权限/会话契约 | Security / Product | 导航 | UI 不判断角色 | Open |
| TBD-008 | 文案 | Username/Password/Login 是否本地化未知 | S-02 为英文 | 产品语言策略 | Product / UX | 文案验收 | 保留样图英文 | Open |
| TBD-009 | 窗口 | 启动透明、无边框、最大化或普通窗口未知 | 栅格无系统边框 | 窗口规范 | UX / App Lead | 最终外观 | 普通可缩放预览，浅色承载透明 PNG | Open |

## 9 工作包与修改边界

| Work Package | Page / Ref | 目标 | allowlist | denylist / 不变量 | 阻塞 TBD | 验证 |
|---|---|---|---|---|---|---|
| WP-01-SPEC | S-01/S-02 | 机器/人工可读规格与映射 | `docs/` | 不改原 PNG | none | JSON parse、链接与哈希核对 |
| WP-02-TEMPLATE | 两页 | Qt 5/6 编译骨架 | `CMakeLists.txt`、`build.ps1`、`src/`、`resources/` | 不接业务/网络/硬件；不复用 `TDLAS_UI_Template` 源码 | none | Release build |
| WP-03-CAPTURE | 两页 | 候选截图和结构检查 | `artifacts/captures/` | 不把候选图冒充原图 | TBD-003（最终） | Windows QPA capture |
| WP-04-INTEGRATE | 两页 | 接入正式工程 | TBD | 不变更认证/权限契约 | TBD-001/002/005/006/007/009 | 目标工程构建/测试/截图 |

## 10 验证与截图矩阵

| VER ID | 类型 | 工作目录 | 命令 / 操作 | 通过标准 | 证据 | 实际结果 |
|---|---|---|---|---|---|---|
| VER-001 | configure/build | 模板根目录 | `.\build.ps1 -QtRoot C:\Qt\6.8.3\msvc2022_64` | exit 0，生成 Release EXE | 构建日志 / EXE | PASS，2026-11-02 |
| VER-002 | runtime capture | 模板根目录 | EXE `--screen splash|login --capture ...`，Windows QPA | 两状态 exit 0，PNG 非空 | CAP-001/CAP-002 | PASS |
| VER-003 | visual structure | 候选图 | 人工逐图检查 | 无结构缺失、拉伸、裁剪、明显溢出或缺字 | CAP-001/CAP-002 | PASS（结构级） |
| VER-004 | offscreen diagnostic | 模板根目录 | EXE `-platform offscreen ...` | 字体正常 | 诊断图 | FAIL；已记录 DIF-002，不作验收基准 |
| VER-005 | target visual QA | 正式工程 | TBD | 无 S0/S1；目标环境校准完成 | TBD | NOT RUN，缺 TBD-001/003 |
| VER-006 | authentication | 正式工程 | TBD | 安全契约和回归通过 | TBD | NOT RUN，缺 TBD-006 |
| VER-007 | standalone deployment | 模板根目录 | `.\deploy.ps1`；移除 Qt PATH 后启动 `dist` EXE 并截图 | 必要 Qt/平台/MSVC DLL 齐全；exit 0 | `dist/`、`deployed-login-final.png` | PASS，修复缺少 `Qt6Widgets.dll` |

| Capture ID | Page / State / Ref | 客户区 candidate | Qt / 平台 | 字体 / 主题 / 语言 | 路径 | 比较结果 |
|---|---|---:|---|---|---|---|
| CAP-001 | Startup / brand / S-01 | widget 请求 1500×750；PNG 1875×938 device px | Qt 6.8.3 / Windows QPA | raster / light / 中文品牌图 | `artifacts/captures/splash-windows.png` | 原图等比复用；Qt capture 映射与外缘承载色待确认 |
| CAP-002 | Login / idle / S-02 | widget 请求 1504×847；PNG 1880×1055 device px | Qt 6.8.3 / Windows QPA | system fallback / light / 英文 | `artifacts/captures/login-windows-v2.png` | 结构接近；实控件表面有 S2 差异；正式 DPR 仍 TBD |

## 11 交付结论

- 完成：原图登记、ARX system 规格、Qt UI_SPEC、token、坐标映射、独立 Qt 5/6 工程、Release 构建、完整 `dist` 部署、无 Qt PATH 启动验证和 Windows QPA 候选截图。
- 未完成：正式工程接入、真实启动/认证/权限、目标 DPI 与字体校准、最终像素级验收。
- allowlist 外变化：更新上级 `CODEX_UI_WORKFLOW.md` 的启动批次状态与实际结果；不修改原图或 `TDLAS_UI_Template`。
- 回退：删除 `codex-qt-template/` 即可；原始样图未改。
