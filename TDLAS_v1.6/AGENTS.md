# TDLAS v1.6 开发约定

本目录是当前开发版本，由 v1.5 扩展而来；保留相邻的 `TDLAS_v1.5/` 原版。根目录 AGENTS.md 中关于 C++17、Qt、NI-DAQmx、算法资料和硬件测试的约定继续适用。

- 工程结构、完整测试命令见 `PROJECT_STRUCTURE.md`，用户操作见 `TDLAS_v1.6发布包/使用说明.txt`。
- Qt 依赖新增 Multimedia（验证环境仍为 Qt 6.11.2 / MSVC 2022 x64）。保持四个 CMake 目标边界；摄像模块目前编入 `tdlas_ui`。
- 摄像窗口由主界面“峰值数据”旁的“摄像”按钮按需打开，不在启动时自动占用摄像头。
- Windows 使用 QCamera / QVideoSink 接收视频，通过 DirectShow IAMCameraControl / IAMVideoProcAmp 查询和设置标准 UVC 参数。按设备 ID 匹配，不硬编码 USB 标识或量程。不支持的控制必须禁用。
- 镜头调焦、变焦使用设备刻度，未标定毫米焦距；软件位移、旋转不移动实体镜头。照片保持完整视频帧尺寸，使用与预览相同的变换逻辑。
- 图片与视频保存目录分别通过 QSettings 记忆。录像由 `CameraRecorder` 流式编码 MP4/H.264，20 fps 目标、无音频；暂停时间从媒体时间线排除，停止或关闭窗口时完成文件保存。录制中所有 UVC/图像调节、画面变换和拍照继续可用。
- 录像始终使用开始录制时预览帧尺寸；已有文件不覆盖。录像相关测试须确认暂停时间、解码播放、变换写入、停止和关闭保存。
- 关闭摄像窗口、Esc、切换设备或主程序退出均须释放摄像头。禁止后台隐藏预览仍占用摄像头。
- 摄像硬件实测不是默认测试步骤。默认 `tdlas_camera_test` 不启动摄像头，`--hardware <目录>` 会实拍、临时修改并恢复亮度，须有用户对当前硬件测试的授权。不启动 NI 采集或 AO 输出。
- 任何代码修改后执行相关 MSVC 构建；UI 修改后检查启动。默认回归使用 `build-tests`、`TDLAS_ENABLE_DAQMX=OFF`，完整运行五项 CTest。
- 构建当前硬件版本：在本目录用根 AGENTS.md 的 CMake 配置命令生成 `build-msvc`，然后执行 `cmake --build build-msvc --config Debug --parallel 4`；发布使用 Release，并部署 Qt Multimedia/FFmpeg 插件和运行库。
- 构建产物、摄像实拍测试图片和日志放在忽略的 `build-*` 中，不把现场照片加入源码或发布包。
