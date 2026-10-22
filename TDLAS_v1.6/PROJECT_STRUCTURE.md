# TDLAS 测氢 v1.6 项目结构

```text
TDLAS_v1.6/
├─ CMakeLists.txt
├─ app/                       # QApplication 和程序入口
├─ include/                   # core、daq、processing、camera、ui 接口
├─ src/                       # 通用模块、DAQmx、算法和 Qt UI 实现
├─ tests/                     # 硬件无关测试
├─ build-msvc/                # 本地 MSVC 构建目录（不提交）
├─ build-tests/               # 本地测试构建目录（不提交）
├─ TDLAS_v1.6发布包/           # v1.6 可直接运行的 Release 包及依赖
└─ 吸收曲线_时域转频域cm-1/    # MATLAB、LVM、CSV 和 Voigt 参考资料
```

## 模块边界

- `tdlas_core`：通用数据模型、峰值历史、TDLAS 算法和独立处理线程，不依赖 DAQmx。
- `tdlas_daq`：封装 NI-DAQmx 设备枚举、连续采集和 AO 波形输出。
- `tdlas_ui`：负责 Qt 主窗口、实时曲线、峰值数据和摄像窗口；使用 Qt Multimedia 采集图像，Windows DirectShow 标准 UVC 接口控制镜头及图像参数。
- `tdlas_app`：创建 `QApplication` 和主窗口。

## 配置说明

默认开启 `TDLAS_ENABLE_DAQMX`。如果开发机尚未安装 NI-DAQmx C API，可以先关闭硬件支持验证 Qt 工程：

```powershell
cmake -S . -B build-msvc -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_PREFIX_PATH="C:/Qt/6.11.2/msvc2022_64" `
  -DTDLAS_ENABLE_DAQMX=OFF
cmake --build build-msvc --config Debug --parallel 4
```

实际设备和通道名称始终由运行时枚举结果决定，程序不会写死设备别名。

## 摄像模块

- `include/camera/uvc_controls.h`、`src/camera/uvc_controls.cpp`：根据摄像头设备路径匹配 UVC 控制接口，查询实际量程、步长及自动/手动模式；不支持的参数在界面中禁用。
- `include/camera/camera_image.h`、`src/camera/camera_image.cpp`：预览及照片共用的位移旋转绘制、PNG/JPEG/BMP 原子保存。
- `include/ui/camera_window.h`、`src/ui/camera_window.cpp`：设备和视频格式选择、实时预览、镜头与图像调节、拍摄、照片保存和目录设置；关闭窗口、按 Esc 或主程序退出均释放摄像头。
- `include/camera/camera_recorder.h`、`src/camera/camera_recorder.cpp`：MP4/H.264 流式录像、暂停后续录、旋转/位移渲染、磁盘及编码错误处理；停止或关闭窗口时完成文件保存。
- 摄像窗口按需创建，程序启动时不会自动启用摄像头。点击主界面“峰值数据”旁的“摄像”后开始预览。录像按 20 fps 目标编码、无音频；录像期间的镜头与图像参数调整继续生效。图片和视频保存目录分别记忆。
- 默认选择可用的 1080p MJPEG 较高帧率档，支持手动选择设备报告的其他格式（含 4K）。预览界面最多以 20 Hz 刷新，照片分辨率保持选定视频帧尺寸；90° 旋转时交换宽高。
- 参数值为摄像头自身刻度，未对 3.41–10.33 mm 焦距进行标定；界面位移和旋转只改变图像，不移动物理镜头。
- Windows 下使用标准系统 UVC 驱动；其他平台仍可预览/拍照，但这版未实现跨平台 UVC 参数控制。

## 验证

Qt 依赖为 Qt 6.8+ 的 Core、Widgets、Charts、Multimedia；本机验证版本 Qt 6.11.2 / MSVC 2022 x64。摄像录像测试使用合成画面验证 MP4/H.264 解码、暂停时间、旋转及关闭保存；不需要启动真实摄像头。

```powershell
$env:PATH = 'C:\Qt\6.11.2\msvc2022_64\bin;' + $env:PATH
& 'C:\Qt\Tools\CMake_64\bin\cmake.exe' -S . -B build-tests -G 'Visual Studio 17 2022' -A x64 -DCMAKE_PREFIX_PATH='C:/Qt/6.11.2/msvc2022_64' -DTDLAS_ENABLE_DAQMX=OFF -DTDLAS_BUILD_TESTS=ON
& 'C:\Qt\Tools\CMake_64\bin\cmake.exe' --build build-tests --config Debug --parallel 4
& 'C:\Qt\Tools\CMake_64\bin\ctest.exe' --test-dir build-tests -C Debug --output-on-failure
```

`tdlas_camera_test` 默认不启动摄像头。仅在用户授权真实摄像测试后，才能手动使用 `tdlas_camera_test.exe --hardware <输出目录>`；该模式会预览外接 4K ZOOM 摄像头、临时修改并恢复亮度、保存照片和视频及截图，并验证录制期间调节可用。任何测试均不启动 NI 采集或输出。
