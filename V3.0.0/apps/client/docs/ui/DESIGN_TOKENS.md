# TDLAS 客户 UI Design Tokens

- 规格版本：0.1.0 Draft。
- 参考 Ref：C-01～C-03；Windows + Qt 6.8.3 只是本轮验证环境。
- 正式主题事实源未提供；当前候选集中在 `resources/styles/client.qss`。
- 实现：单一作用域 QSS + C++ 自绘图表/预览。

## Token 清单

| Token | 语义 | 值 | 单位 | 证据 | 实现映射 |
|---|---|---:|---|---|---|
| `color.action.primary` | 主操作/选中页 | `#087CF0` | sRGB | ESTIMATED | `client.qss` |
| `color.navigation.deep` | 左侧栏 | `#0755A9` | sRGB | ESTIMATED | `#sideRail` |
| `color.canvas` | 主画布 | `#EDF7FF` | sRGB | ESTIMATED | 根/页面背景 |
| `color.surface` | 卡片表面 | `#FFFFFF` | sRGB | ESTIMATED | panel |
| `color.panelHeader` | 面板标题条 | `#EDF7FF` | sRGB | ESTIMATED | `panelTitle` |
| `color.border` | 卡片/控件边框 | `#DBEAF6` | sRGB | ESTIMATED | panel/controls |
| `color.text.primary` | 主要文字 | `#152C49` | sRGB | ESTIMATED | 根文字 |
| `color.chart.primary` | 主曲线 | `#0879EF` | sRGB | ESTIMATED | `ChartWidget` |
| `space.xs` | 紧密间距 | 6 | Qt logical px candidate | INFERRED | layouts |
| `space.sm` | 控件内/间距 | 10 | Qt logical px candidate | INFERRED | layouts |
| `space.md` | 面板间距 | 12 | Qt logical px candidate | INFERRED | layouts |
| `space.lg` | 页面边距 | 16 | Qt logical px candidate | INFERRED | layouts |
| `radius.control` | 输入/按钮 | 7 | Qt logical px candidate | ESTIMATED | QSS |
| `radius.panel` | 卡片 | 11 | Qt logical px candidate | ESTIMATED | QSS |
| `radius.overlay` | 浮层 | 14 | Qt logical px candidate | ESTIMATED | QSS |
| `size.topBar` | 顶栏 | 64 | Qt logical px candidate | ESTIMATED | C++ |
| `size.sideRail` | 侧栏 | 88 | Qt logical px candidate | ESTIMATED | C++ |
| `size.tabBar` | 页签条 | 64 | Qt logical px candidate | ESTIMATED | C++ |
| `font.ui` | 中文 UI fallback | `Microsoft YaHei UI`, `Segoe UI` | family | INFERRED | QSS |

## 状态映射

| 组件 / uiRole | normal | hover | focus / pressed | disabled | selected | 实现 |
|---|---|---|---|---|---|---|
| `topTab` | 白蓝表面 | 浅蓝 | 保留原生焦点 | 降对比 | 蓝色渐变 | QSS |
| `railAction` | 透明白字 | 浅白表面 | 保留原生焦点 | 降对比 | 浅白底/蓝字 | QSS |
| `primary` | 蓝底白字 | 提亮蓝 | 保留原生焦点 | 降对比 | N/A | QSS |
| `secondary` | 浅底蓝字 | 浅蓝 | 保留原生焦点 | 灰底灰字 | N/A | QSS |
| 输入控件 | 白底浅蓝边框 | 原生 | 2 px 蓝边框 | Qt 原生 | N/A | QSS |
| `movement` | 浅蓝表面 | 蓝色加深 | 蓝色加深 | N/A | N/A | QSS |

## 样式边界

- 选择器限定于 `#clientTemplateRoot`、页面 objectName 和明确 `uiRole`。
- 不定义自绘 `windowAction`；最小化、最大化和关闭使用 Windows 原生标题栏控件。
- 未打包字体；中文 fallback、许可与正式缩放由 TBD-C-003/C-010 关闭。
- 当前图标以文本/基础符号表达；正式工程应映射许可明确且支持高 DPI 的 SVG。
- 样图没有证明 hover/focus/disabled/error/busy 的精确样式；当前状态均为可回退候选。
