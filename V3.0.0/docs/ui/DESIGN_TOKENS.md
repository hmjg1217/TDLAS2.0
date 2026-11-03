# TDLAS Hydrogen UI Suite 设计令牌边界

- 规格版本：0.2.0 Implemented / 2026-11-02。
- 本轮没有统一三个样板的视觉主题。
- 启动端事实源：apps/startup/resources/styles/startup.qss 与 apps/startup/docs/ui/DESIGN_TOKENS.md。
- 管理员端事实源：apps/admin/resources/styles/admin.qss 与 apps/admin/docs/ui/DESIGN_TOKENS.md。
- 客户端事实源：apps/client/resources/styles/client.qss 与 apps/client/docs/ui/DESIGN_TOKENS.md。

| Token / 资产 | 值 | 证据 | 实现映射 | fallback / 限制 |
|---|---|---|---|---|
| sidebar.icon.home | :/sidebar-icons/home.png | CONFIRMED | shared/resources/sidebar-icons.qrc | 当前未绑定按钮 |
| sidebar.icon.parameters | :/sidebar-icons/parameters.png | CONFIRMED | 同上 | 当前未绑定按钮 |
| sidebar.icon.start | :/sidebar-icons/start-acquisition.png | CONFIRMED | 同上 | 当前未绑定按钮 |
| sidebar.icon.stop | :/sidebar-icons/stop-acquisition.png | CONFIRMED | 同上 | 当前未绑定按钮 |
| sidebar.icon.save | :/sidebar-icons/save-data.png | CONFIRMED | 同上 | 当前未绑定按钮 |
| sidebar.icon.size | 未确认 | TBD-INT-004 | 后续按钮 QIcon::setIconSize | 不在本轮猜测 |

## 样式边界

- 整合主程序在同一 QApplication 内按当前页面只加载一份 QSS：登录为 startup.qss，角色路由成功后替换为 admin.qss 或 client.qss；不拼接样式。
- 三个预览程序仍各自加载原 QSS，用于页面截图回归。
- 五张侧栏 PNG 是 1254×1254 ARGB；高 DPI 缩放由 QIcon 负责，最终逻辑尺寸待 UX/目标设备校准。
- 本轮不修改 normal、hover、pressed、focus、disabled、selected 状态映射。
- 正式字体、品牌许可和中文 fallback 仍以各样板 TBD 为准。
