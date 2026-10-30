# TDLAS2.0

TDLAS 测氢 Windows 桌面程序，使用 C++17、Qt 6 Widgets/Charts/Multimedia、CMake、Visual Studio 2022 x64 和 NI-DAQmx ANSI C API。

当前源码位于 [`TDLAS2.0/`](TDLAS2.0/)。项目固定编译 NI-DAQmx 后端，同时保留模拟数据源用于硬件无关测试；程序启动不会自动枚举设备、采集、输出 AO 或打开摄像头。

## 构建

1. 将 `TDLAS2.0/tools/local-env.example.json` 复制为 `local-env.json`，填写本机 Qt、CMake、Visual Studio、Windows SDK 和 NI-DAQmx SDK 路径。
2. 运行 `TDLAS2.0/Build-Debug.cmd --no-pause` 构建 Debug 并执行测试。
3. 运行 `TDLAS2.0/Build-Release.cmd --no-pause` 构建、测试并生成可移植目录和 ZIP。

目标机须为 64 位 Windows，并安装与采集卡匹配的 NI-DAQmx 驱动。Qt、FFmpeg 和 MSVC 运行库由 Release 部署脚本一并打包。

详细信息见 [项目说明](TDLAS2.0/README.md)、[用户使用说明](TDLAS2.0/docs/用户使用说明.md) 和 [实施计划](TDLAS2.0/docs/实施计划.md)。v1.5、v1.6 保留在 Git 历史和标签中，不再复制到当前源码树。
