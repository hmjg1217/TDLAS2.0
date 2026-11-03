# TDLAS Hydrogen UI Suite Reference Manifest

## 目标环境

| 项目 | 值 | 证据 | 来源 / TBD |
|---|---|---|---|
| 平台 / Qt / 编译器 | Windows / Qt 6.8.3 / MSVC 2022 x64 候选 | CONFIRMED（原样板） | 正式环境 TBD-INT-002 |
| 目标客户区 | 启动页与两端页面分别沿用原样板候选 | INFERRED | apps/*/docs/ui/reference/MANIFEST.md |
| OS 缩放 / DPI / DPR | 未确认 | TBD | TBD-INT-002 |
| 字体 / 主题 / 语言 | 各样板原 QSS；浅色 / zh-CN + 英文占位 | INFERRED | 各样板原规格 |
| 捕获方式 | QWidget::grab()，命令行 --capture | CONFIRMED | 整合主程序与三个预览入口 |

## 页面参考图

| Ref ID | 整合工程路径 | 页面 / 状态 | 像素 | SHA-256 | 证据 |
|---|---|---|---:|---|---|
| S-01 | apps/startup/resources/reference/startup_splash.png | 启动展示 | 1774×887 ARGB | BA5DEC8F8D8A498E5DCE84C7EC4D0BDD8E255969389D3F0D6E6E3EB73D19A283 | CONFIRMED |
| S-02 | apps/startup/resources/reference/login_reference.png | 登录 idle | 1671×941 RGB | FBE03981A6BF62DEABB8E34726B8FDEB495174AF84BE417E2FE73F6A17E44EBB | CONFIRMED |
| A-01 | apps/admin/resources/reference/a01-main.png | 管理员主页面 | 1672×941 RGB | 9B8ED536D399FC3C10B1EE17B925A21D7E3B5A8C39F4A9861CDC4336D6C7FB75 | CONFIRMED |
| A-02 | apps/admin/resources/reference/a02-main-parameters.png | 管理员参数抽屉 | 1672×941 RGB | 8880992D1220C2EED0F0D5C9C9FABB1CDAF53E4E3216E13A3C892E2EFDC277EA | CONFIRMED |
| A-03 | apps/admin/resources/reference/a03-debug.png | 管理员调试页 | 1672×941 RGB | 5883A7ABED36B3CACE36A3B59362523784FD5EDB8864BE836938BE83CC1D612B | CONFIRMED |
| A-04 | apps/admin/resources/reference/a04-peak-data.png | 管理员峰值页 | 1672×941 RGB | 7CF0F3930D0B881A7A0939E7841327E7B20175A23FC81D26C3197B03A40AD47B | CONFIRMED |
| A-05 | apps/admin/resources/reference/a05-camera.png | 管理员摄像页 | 1671×941 RGB | 65BB5BB085705DC304D5D80CEACB5B052FB8970FD3FCD71A4732EB3485498AF8 | CONFIRMED |
| A-06 | apps/admin/resources/reference/a06-camera-motion.png | 管理员摄像浮层 | 1671×941 RGB | 77A205BD34776B0C75B5E5C4E56C440ECB59C2E0C2CDF0E1923C163F0CEB3A6E | CONFIRMED |
| C-01 | apps/client/resources/reference/c01-main.png | 客户主页面 | 1672×941 RGB | 055B41394AFCA0E9182C90BE978210A5F473227AB48F85F4E86A3FFFB7207E2E | CONFIRMED |
| C-02 | apps/client/resources/reference/c02-camera.png | 客户摄像页 | 1671×941 RGB | 18659BAC8D9B59ED5275BD2C608E6A678DB4F538681CE7484AE8CA9B3F1DD9DA | CONFIRMED |
| C-03 | apps/client/resources/reference/c03-camera-motion.png | 客户摄像浮层 | 1670×941 RGB | 58E5D5BDF2718053C9BB15936F7A85EA39F6DE89D6E82CDB362CC60388C46508 | CONFIRMED |

这些生成参考图没有可证明的真实 DPR；画布像素不能直接当作 Qt 逻辑客户区。更细的校准锚点和替代关系保留在各 app 的原 MANIFEST。

## 共享侧栏资产

| Asset ID | 整合工程路径 | Qt alias | 像素 | SHA-256 | 当前采用 |
|---|---|---|---:|---|---|
| ICON-01 | shared/resources/sidebar-icons/主页面.png | :/sidebar-icons/home.png | 1254×1254 ARGB | 55A618AD244AFBED56BAF219244CA6E9F3DF4A8CA7A98D9E6039D4509C462190 | integrated/admin/client QRC；未绑定按钮 |
| ICON-02 | shared/resources/sidebar-icons/参数区.png | :/sidebar-icons/parameters.png | 1254×1254 ARGB | 4CB15378D96FD54C4AAA2FB5CAD4F8C906BA0A0C6DDE99CFEDFA913CAAB0871F | 同上 |
| ICON-03 | shared/resources/sidebar-icons/开始采集.png | :/sidebar-icons/start-acquisition.png | 1254×1254 ARGB | F755CBF28126322E6C5E79CAD91F55A7918408D31089BBE9FA5B75E954E63A24 | 同上 |
| ICON-04 | shared/resources/sidebar-icons/停止采集.png | :/sidebar-icons/stop-acquisition.png | 1254×1254 ARGB | 186A727918A841043B73958380A40A3409E2931CA425296A3B03236583BC3EEA | 同上 |
| ICON-05 | shared/resources/sidebar-icons/数据保存.png | :/sidebar-icons/save-data.png | 1254×1254 ARGB | 25B39ACF2AB93CEC001A4808D87E9FDD99627438F3ADFEA56FC2FBBF32EB4F2C | 同上 |

资产来源：E:\AI\氢气3.0\UI设计\控件，用户指定供客户端和管理员端后续调用。本轮内部使用已获授权；正式分发许可记录见 TBD-INT-003。compiled/ 中英文文件名副本与原图逐字节一致，是实际 RCC 输入。

## 原候选截图

| 角色 | 整合工程路径 | 文件数 | 用途 |
|---|---|---:|---|
| 启动/登录 | artifacts/source-baselines/startup | 7 | 原样板结构候选与部署复验 |
| 管理员 | artifacts/source-baselines/admin | 10 | 原样板六状态及修订证据 |
| 客户 | artifacts/source-baselines/client | 11 | 原样板四状态及部署复验 |

## 本次候选截图

本次重新构建生成的 13 个状态截图写入 artifacts/captures：整合登录 1、启动预览 2、管理员 6、客户 4；另含部署版整合登录复验 1 张。全部 exit 0 且文件非空。

- 整合登录与启动预览登录 SHA-256 一致。
- 三个原目录当前 dist EXE 重新捕获的 12 个预览状态存于 artifacts/current-source-captures，与整合工程对应截图逐张 SHA-256 一致。
- 联系表：artifacts/qa/capture-contact-sheet.png；结构级检查无缺失、拉伸或主要溢出。
