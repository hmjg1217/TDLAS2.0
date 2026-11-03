# Sidebar icon assets

来源：E:\AI\氢气3.0\UI设计\控件（用户于 2026-11-02 指定）。

| 原文件 | Qt alias | 尺寸 | SHA-256 |
|---|---|---:|---|
| 主页面.png | :/sidebar-icons/home.png | 1254×1254 ARGB | 55A618AD244AFBED56BAF219244CA6E9F3DF4A8CA7A98D9E6039D4509C462190 |
| 参数区.png | :/sidebar-icons/parameters.png | 1254×1254 ARGB | 4CB15378D96FD54C4AAA2FB5CAD4F8C906BA0A0C6DDE99CFEDFA913CAAB0871F |
| 开始采集.png | :/sidebar-icons/start-acquisition.png | 1254×1254 ARGB | F755CBF28126322E6C5E79CAD91F55A7918408D31089BBE9FA5B75E954E63A24 |
| 停止采集.png | :/sidebar-icons/stop-acquisition.png | 1254×1254 ARGB | 186A727918A841043B73958380A40A3409E2931CA425296A3B03236583BC3EEA |
| 数据保存.png | :/sidebar-icons/save-data.png | 1254×1254 ARGB | 25B39ACF2AB93CEC001A4808D87E9FDD99627438F3ADFEA56FC2FBBF32EB4F2C |

原中文文件完整保留在本目录；compiled/ 中是逐字节相同、仅改为英文文件名的编译副本，用于避免 RCC/MSVC 生成代码的路径编码警告。

当前资源已加入整合主程序、管理员预览与客户端预览的 QRC，但不改变任何按钮外观。五张图含真实透明与半透明像素，只有单一彩色状态。后续接入建议通过 QIcon 和明确的 iconSize 使用；若目标侧栏为 24–64 逻辑像素，应先制作经视觉校准的派生尺寸和 selected/disabled/hover 状态，不在代码中依赖原中文文件名。
