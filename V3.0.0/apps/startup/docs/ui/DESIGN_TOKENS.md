# UI Design Tokens

- 规格版本：0.1.0 Draft
- 参考 Ref / 基准环境：S-01、S-02；Windows + Qt 6.8.3 仅为本轮验证环境
- 项目主题事实源：正式工程未提供；当前候选值集中在 `resources/styles/startup.qss`
- 实现方式：单一作用域 QSS + C++ 参考画布

## Token 清单

| Token | 层级 / 语义 | 值 | 单位 | 主题 / 状态 | 证据状态 | 来源 / TBD ID | QSS 或 C++ 映射 | 校准 / fallback |
|---|---|---:|---|---|---|---|---|---|
| `color.canvas.splash` | 启动画布背景 | `#F4FAFF` | sRGB | light | INFERRED | S-01 透明边缘的承载色候选 | `paintEvent` | 正式宿主背景可替换 |
| `color.canvas.login` | 登录 letterbox 背景 | `#DCEFFC` | sRGB | light | ESTIMATED | S-02 边缘采样 | `paintEvent` | 与目标窗口背景校准 |
| `color.surface.loginField` | 实控件不透明表面 | `#EEF7FD` | sRGB | normal | ESTIMATED | S-02 表单区域采样 | `startup.qss` | 用于遮盖烘焙表单 |
| `color.brand.deepBlue` | 产品标题 | `#043491` | sRGB | light | ESTIMATED | S-02 栅格采样 | 参考，不重绘 Logo | 品牌源文件优先 |
| `color.action.start` | 登录按钮渐变 | `#06A7E9` | sRGB | normal | ESTIMATED | S-02 按钮视觉 | `startup.qss` | 需目标截图校准 |
| `color.action.end` | 登录按钮渐变 | `#006FF1` | sRGB | normal | ESTIMATED | S-02 按钮视觉 | `startup.qss` | 需目标截图校准 |
| `color.field.divider` | 输入下划线 | `#9AA9CA` | sRGB | normal | ESTIMATED | S-02 栅格观察 | `startup.qss` | focus 使用 `#087EE9` |
| `space.fieldGap` | 两输入框间距 | 18 | Qt logical px candidate | normal | ESTIMATED | S-02 视觉测量 | `QVBoxLayout` | 随目标 DPR 校准 |
| `space.actionGap` | 密码到主按钮间距 | 34 | Qt logical px candidate | normal | ESTIMATED | S-02 视觉测量 | `QVBoxLayout` | 随目标 DPR 校准 |
| `size.loginField` | 输入高度 | 66 | fitted-reference px candidate | normal | ESTIMATED | S-02 `ui-map.json` | `updateLoginFormGeometry` | 最小 42 logical px |
| `size.loginButton` | 按钮高度 | 61 | fitted-reference px candidate | normal | ESTIMATED | S-02 `ui-map.json` | `updateLoginFormGeometry` | 最小 42 logical px |
| `radius.loginButton` | 主按钮圆角 | 14 | Qt logical px candidate | normal | ESTIMATED | S-02 视觉观察 | `startup.qss` | 目标截图校准 |
| `font.ui` | 表单字体 | `Microsoft YaHei UI`, `Segoe UI` | family fallback | light | INFERRED | 原图字体 UNKNOWN | `startup.qss` | 以目标工程字体为准 |

## 状态映射

| 组件 / objectName / uiRole | normal | hover | pressed | focus | disabled | selected / error / busy | 实现位置 |
|---|---|---|---|---|---|---|---|
| `usernameEdit`, `passwordEdit` / `loginField` | 浅色实面 + 灰蓝下划线 | 下划线加深 | N/A | 2 px 蓝色下划线 | 文本与线降对比 | 错误由状态文案表达；busy 禁用 | `startup.qss`、`setLoginBusy` |
| `loginButton` | 青到蓝渐变 | 渐变提亮 | 深蓝实色 | 白色焦点边框 | 灰蓝表面 | busy 文案 `Signing in…` | `startup.qss`、`setLoginBusy` |
| `loginMessage` | 灰蓝文字 | N/A | N/A | N/A | N/A | error 使用暗红文字 | `startup.qss`、`setLoginMessage` |

## 样式边界

- 选择器作用域：`#rootCanvas`、`#loginForm`、明确 objectName 或 `uiRole`；不应污染目标工程其他页面。
- 动态属性刷新：`setLoginMessage` 对 `severity` 执行 unpolish/polish。
- 图标：账号与锁图标由 `QPainter` 绘制，可随高 DPI 重新生成；正式工程可换成已许可 SVG。
- 字体：不打包字体；中文 fallback 和许可证均由目标工程确认（TBD-003）。
- 未展示状态：原图没有 hover、pressed、focus、disabled、busy 或 error；模板状态是可回退的实现候选，不是源图事实。
