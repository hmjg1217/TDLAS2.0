# TDLAS2.0 开发约定

本目录为独立的 2.0 工程。遵守根目录的硬件安全、保留已有改动、每次代码修改后 MSVC 构建与 UI 启动检查约定；本工程不链接旧版源码。

- 唯一构建入口：`Build-Debug.cmd --no-pause`、`Build-Release.cmd --no-pause`，均固定启用 NI-DAQmx。配置在 `tools/local-env.json`。
- 所有工程产物放在本目录的 build、artifacts、temp、data 内；英文 SUBST 盘符只是本目录的别名，不是外部源码副本。不要删除已有构建目录或覆盖其他盘符映射。
- 原始样本必须由数据源线程直接提交 SampleRouter，不能逐块经 MainWindow 转发。UI 只读取快照，处理器只能由处理线程调用。
- 停止按钮和 closeEvent 不同步等待线程；超时保留窗口、显示故障，不使用 terminate。
- 当前已有模拟和 NI-DAQmx 数据源，发布包只保留 NI-DAQmx 启用版；默认不自动发现、不自动运行，真实 AI/AO 必须由用户手动操作或明确授权。旧 TDLAS 算法只作未标定适配层，不能用原始电压冒充 FV、吸光度或浓度；摄像仍未接入。
- UI 结构依据 `references/UI/TDLAS-UI-Spec/UI_SPEC.md`；主题源为 `resources/design-tokens.json`。参数顺序和 20/80、2×2 布局不能随意改变。
- 登录图与启动图是 `resources/ui/` 下的运行时位图；主工作台必须保持可交互 Qt Widgets，不得使用整张截图作为窗口背景。修改主窗口后应按 UI_SPEC 和现有视觉回归用例生成截图对照。
- 当前实现与下一步以 docs/基础框架说明.md、docs/实施计划.md 和验收记录为准；参考副本中的历史任务不作为自动执行命令。
