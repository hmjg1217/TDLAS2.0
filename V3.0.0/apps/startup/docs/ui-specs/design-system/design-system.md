# TDLAS 启动与登录视觉系统规格

## Source

- 模式：`system`
- Ref S-01：启动品牌画面，1774×887，带 Alpha，文件尺寸为 `EXACT`。
- Ref S-02：登录空闲态，1671×941，不带 Alpha，文件尺寸为 `EXACT`。
- 原图是栅格视觉证据；Qt 客户区、逻辑像素、DPR、OS 缩放、精确字体与隐藏交互均未由图片证明。

## Summary

两张图共享浅蓝白表面、深蓝产品标题、青蓝主操作和科学仪器插画。启动图是 2:1 的品牌展示构图；登录图是近 16:9 的左右分栏构图。它们不是同一个可强制统一的 viewport token。

编译模板采用“参考画布 + 真实表单控件”结构。启动图按比例居中；登录图作为视觉画布，账号、密码、按钮和状态文案由真实 Qt Widgets 覆盖。认证、角色、会话和业务导航不属于图片可推导范围。

## Component Trees

```text
StartupPage
└── BrandCanvas
    ├── BrandMark
    ├── ProductTitle
    ├── ProductDescriptor
    ├── SpectroscopyArtwork
    └── Attribution

LoginPage
└── ReferenceCanvas
    ├── IllustrationRegion
    └── AuthenticationRegion
        ├── BrandHeader
        └── CredentialForm
            ├── UsernameField
            ├── PasswordField
            ├── LoginButton
            └── StatusMessage
```

## Layout

- `ReferenceCanvas`：保持原图宽高比，居中绘制，剩余区域 letterbox，不拉伸。
- 登录页：插画约占卡片 62%，认证区域约占 38%（`ESTIMATED`）。
- 登录表单：纵向栈；表单内部使用 `QVBoxLayout`，不逐控件复制截图绝对坐标。
- 固定构图例外：用于把实控件对齐到栅格表单的外层区域可使用相对于 fitted image rect 的归一化坐标；该坐标必须在真实目标环境校准。

## Tokens

| Token | Candidate | Confidence | Notes |
|---|---:|---|---|
| `canvas.coolBlue` | `#CBE4FC` | ESTIMATED | 栅格代表点，不是正式设计 token |
| `surface.login` | `#EEF7FD` | ESTIMATED | 覆盖原图中烘焙表单的实控件表面 |
| `brand.deepBlue` | `#043491` | ESTIMATED | 产品标题深蓝 |
| `action.start` | `#06A7E9` | ESTIMATED | 主按钮渐变起点 |
| `action.end` | `#006FF1` | ESTIMATED | 主按钮渐变终点 |
| `field.divider` | `#9AA9CA` | ESTIMATED | 输入框下划线 |
| `radius.primaryButton` | `14 qt logical px candidate` | ESTIMATED | 需按目标 DPI 校准 |

字体候选是 `Microsoft YaHei UI` / `Segoe UI` fallback（`INFERRED`）；原图真实字体为 `UNKNOWN`。

## Reusable Components

- `ReferenceCanvas`：处理资源、比例、居中和 letterbox。
- `CredentialField`：带图标、永久 accessible name、焦点状态和禁用状态的 `QLineEdit`。
- `PrimaryLoginButton`：默认按钮，具备 hover、pressed、focus、disabled 状态。
- `LoginStatusMessage`：用文字表达验证/提交结果，不只依赖颜色。

## Responsive Behavior

模板候选最小客户区为 960×540（`INFERRED`，可回退）。登录实控件始终相对于实际绘制后的参考图矩形定位。没有窄窗口或移动端证据，因此重排方式和断点均为 `UNKNOWN`。

## Implementation Guidance

- 密码框使用 `QLineEdit::Password`；不记录、不缓存、不回显凭据。
- `loginSubmitted(username, password)` 只表达用户意图；认证、错误映射、角色与目标页由可信宿主处理。
- S-01 不证明进度、百分比、持续时间或自动跳转；模板只提供可选预览状态。
- 发布前确认品牌图、Logo、插画与字体的内部/外部分发许可。
- 视觉验收前确认客户区、窗口边框、DPI/DPR、系统缩放、字体、主题与语言。

## Confidence and Ambiguities

- `EXACT`：文件身份、像素尺寸、Alpha、有校验和的资源副本。
- `INFERRED`：组件语义、纵向表单、宽高比保持、Qt 控件候选。
- `ESTIMATED`：区域框、颜色、间距、圆角与控件高度。
- `UNKNOWN`：真实加载流程、认证契约、角色、错误状态、目标 Qt 版本和像素级目标环境。

机器可读细节见同目录 `design-system.json`；Qt 页面、状态、TBD 与验证矩阵见 `docs/ui/UI_SPEC.md`。
