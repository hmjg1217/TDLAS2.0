# TDLAS 管理员 UI Design Tokens

- 规格版本：0.1.0 Draft
- 参考 Ref：A-01～A-06；Windows + Qt 6.8.3 只是本轮验证环境。
- 项目主题事实源：正式工程未提供；当前候选集中在 `resources/styles/admin.qss`。
- 实现方式：单一作用域 QSS + C++ 自绘图表/预览。

## Token 清单

| Token | 语义 | 值 | 单位 | 证据状态 | 来源 / TBD | 实现映射 |
|---|---|---:|---|---|---|---|
| `color.action.primary` | 主操作/选中页 | `#087CF0` | sRGB | ESTIMATED | A-01～A-06 | `admin.qss` |
| `color.navigation.deep` | 左侧栏 | `#0755A9` | sRGB | ESTIMATED | A-01～A-06 | `#sideRail` |
| `color.canvas` | 主画布 | `#EDF7FF` | sRGB | ESTIMATED | 栅格采样 | 根/页面背景 |
| `color.surface` | 卡片表面 | `#FFFFFF` | sRGB | ESTIMATED | 重复卡片 | panel/table |
| `color.panelHeader` | 面板标题条 | `#EDF7FF` | sRGB | ESTIMATED | 重复标题条 | `panelTitle` |
| `color.border` | 卡片/表格边框 | `#DBEAF6` | sRGB | ESTIMATED | 栅格采样 | panel/grid |
| `color.text.primary` | 主要文字 | `#152C49` | sRGB | ESTIMATED | 栅格采样 | 根文字 |
| `color.chart.primary` | 主曲线 | `#0879EF` | sRGB | ESTIMATED | 图表视觉 | `ChartWidget` |
| `color.chart.fit` | 拟合曲线 | `#EC3F4F` | sRGB | ESTIMATED | A-03 | `ChartWidget` |
| `color.status.error` | 无效/错误 | `#D92C36` | sRGB | ESTIMATED | A-04 | 表格状态 |
| `space.xs` | 紧密间距 | 6 | Qt logical px candidate | INFERRED | 重复聚类 | QSS/layout |
| `space.sm` | 控件内/间距 | 10 | Qt logical px candidate | INFERRED | 重复聚类 | layouts |
| `space.md` | 面板间距 | 12 | Qt logical px candidate | INFERRED | 重复聚类 | layouts |
| `space.lg` | 页面边距 | 16 | Qt logical px candidate | INFERRED | 重复聚类 | layouts |
| `radius.control` | 输入/按钮 | 7 | Qt logical px candidate | ESTIMATED | 栅格观察 | QSS |
| `radius.panel` | 卡片 | 11 | Qt logical px candidate | ESTIMATED | 栅格观察 | QSS |
| `radius.overlay` | 浮层 | 14 | Qt logical px candidate | ESTIMATED | A-02/A-06 | QSS |
| `size.topBar` | 顶栏 | 64 | Qt logical px candidate | ESTIMATED | 模板候选 | C++ |
| `size.sideRail` | 侧栏 | 88 | Qt logical px candidate | ESTIMATED | 模板候选 | C++ |
| `size.tabBar` | 页签条 | 64 | Qt logical px candidate | ESTIMATED | 模板候选 | C++ |
| `font.ui` | 中文 UI fallback | `Microsoft YaHei UI`, `Segoe UI` | family | INFERRED | 精确字体 UNKNOWN | QSS |

## 状态映射

| 组件 / uiRole | normal | hover | pressed/focus | disabled | selected/error/busy | 实现位置 |
|---|---|---|---|---|---|---|
| `topTab` | 白蓝表面 | 浅蓝 | 蓝色焦点由原生控件保留 | 降对比 | 蓝色渐变选中 | `admin.qss` |
| `railAction` | 透明白字 | 浅白表面 | 原生焦点 | 降对比 | 浅白底/蓝字 | `admin.qss` |
| `primary` | 蓝底白字 | 提亮蓝 | 原生焦点 | 降对比 | N/A | `admin.qss` |
| `secondary` | 浅底蓝字 | 浅蓝 | 原生焦点 | 灰底灰字 | N/A | `admin.qss` |
| 输入控件 | 白底浅蓝边框 | 原生 | 2 px 蓝边框 | Qt 原生 | 错误态正式接入 TBD | `admin.qss` |
| `movement` | 浅蓝表面 | 蓝色加深 | 蓝色加深 | N/A | N/A | `admin.qss` |

## 样式边界

- 选择器限定于 `#adminTemplateRoot`、页面 objectName、明确 `uiRole` 和模板控件。
- 未打包字体；中文 fallback、许可证和目标缩放由 TBD-A-003/A-010 关闭。
- 当前文本符号只是候选图标；正式工程应映射许可明确的 SVG/图标库。
- 样图没有证明 hover/focus/disabled/error/busy 的精确样式；当前状态均为可回退实现候选。
