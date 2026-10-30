# TDLAS2.0 项目约定

当前开发项目为 `TDLAS2.0/`。v1.5、v1.6 已由 Git 历史保存；本机的 `旧版程序归档/` 仅作本地备份，不再提交。具体模块和 UI 约束见 `TDLAS2.0/AGENTS.md`。

## 技术栈与入口

- C++17、Qt 6 Widgets/Charts/Multimedia、CMake、Visual Studio 2022 x64、NI-DAQmx ANSI C API。
- `TDLAS2.0/Build-Debug.cmd --no-pause`：DAQmx Debug 构建及硬件无关测试。
- `TDLAS2.0/Build-Release.cmd --no-pause`：DAQmx Release 构建、测试和可移植包部署。
- 本机路径写在被忽略的 `TDLAS2.0/tools/local-env.json`；仓库只提交 `local-env.example.json`。

## 硬件安全

- 程序启动不得自动枚举、采集、输出 AO 或打开摄像头。
- 不要为验证界面自动点击“开始采集”。真实 NI AI/AO 和真实摄像头操作必须由用户明确执行或授权。
- 设备和通道必须运行时枚举，不能写死 `Dev1/ai0`。
- 停止通过取消请求和线程排空完成，不使用 `terminate`，不在采集线程外操作 NI 任务句柄。

## 代码与仓库

- 保持 `tdlas_core`、`tdlas_daq`、`tdlas_processing`、`tdlas_storage`、`tdlas_camera`、`tdlas_runtime`、`tdlas_ui` 和 `tdlas_app` 的边界。
- 算法结果在物理定义和标定完成前必须保持“未校准”，不能用原始电压伪造 FV、吸光度或浓度。
- 修改前检查工作区并保留无关用户改动；不使用破坏性 Git 回退。
- 不提交 `build/`、`artifacts/`、`data/`、`temp/`、本机路径配置、测试截图或便携包。
- 代码修改后至少执行相关 MSVC 构建和测试；UI 变更补充启动检查；真实硬件是否访问必须明确说明。
