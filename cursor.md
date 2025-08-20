## 项目概览

- 本仓库基于 Qv2ray，已完成多内核改造：在保留 V2Ray/Xray 适配器的同时，新增 `sing-box` 与 `mihomo`（Clash/Clash Meta）内核适配，并扩展 Reality/Anylts 等现代协议支持。
- 关键集成：
  - `src/core/kernel/PolyCoreKernelManager.{hpp,cpp}`：多内核生命周期、自动发现、版本探测、配置校验、日志/统计转发、首选内核策略。
  - `src/core/kernel/SingBoxKernelAdapter.{hpp,cpp}`：sing-box 配置生成、日志解析、就绪判断、Reality/Anylts/DNS/路由转换。
  - `src/core/kernel/MihomoKernelAdapter.{hpp,cpp}`：mihomo YAML 配置生成、策略组与规则转换、日志解析与就绪判断。
  - `src/core/kernel/V2RayKernelAdapter.{hpp,cpp}`：兼容 v2ray/xray，包含 Xray 的 Reality 处理。
  - `src/core/protocols/ProtocolSupport.{hpp,cpp}`：Reality/Anylts 抽象建模、校验与跨内核配置片段生成能力判定。
  - `src/core/dns/SingBoxDNSConfig.{hpp,cpp}`：sing-box 1.12.0+ DNS 新格式的结构化建模与迁移。
- 构建系统使用 CMake，支持 Qt5/Qt6；Windows/macOS/Linux/Android 的平台部署脚本位于 `cmake/platforms/`。

### 代码结构（高层）
- Base：`src/base/` 应用基类、配置模型、日志与特性宏。
- Core：`src/core/` 业务与内核交互（连接生成、内核适配、设置、路由/DNS）。
- UI：`src/ui/` 三套前端（Widgets/QML/CLI），共用小部件与消息总线。
- Plugins：`src/plugins/` 内置协议与订阅插件（UI 配置器 + 核心适配）。
- Components：网络、统计、更新、代理配置等工具组件。


## 构建与运行（精简）

- 构建（示例，Qt Widgets + Release）：
  - Windows（MSVC 或 MinGW）/ Linux / macOS 通用：
    ```bash
    mkdir build && cd build
    cmake .. -DQV2RAY_UI_TYPE=QWidget -DCMAKE_BUILD_TYPE=Release
    cmake --build . -j
    ```
- 运行时在可执行文件同目录放置内核：
  - `sing-box.exe`（或 `sing-box`）
  - `mihomo-windows-amd64.exe`（或 `mihomo`/`clash`）
- 自动发现路径（代码内已有）：`/usr/bin`, `/usr/local/bin`, `/opt/{v2ray,xray,mihomo,sing-box}`, `~/.local/bin`, `~/.local/share/qv2ray/bin`，Windows 包含程序目录与 `C:/Program Files/...`。

建议补充的发现路径：
- Linux/macOS：`/opt/homebrew/bin`（Apple Silicon），必要时增加可配置的自定义搜索路径。
- Windows：支持相对路径（如 `<程序目录>/core`）。


## 关键行为与能力

- 多内核选择：
  - `PolyCoreKernelManager` 会根据偏好和可用性选择 `V2RAY/XRAY/MIHOMO/SING_BOX`，为所选内核生成配置并启动。
  - 启动参数：
    - sing-box: `run -c <config>`
    - mihomo: `-f <config>`
    - v2ray/xray: `run -c <config>`
- 协议能力判定：
  - Reality：xray、sing-box、mihomo(限 vless) 支持。
  - Anylts：仅 sing-box 支持。
- 配置格式：
  - sing-box 与 v2ray/xray 使用 JSON，mihomo 使用 YAML。
- DNS/路由：
  - sing-box 使用 1.12.0+ 新 DNS 格式，已提供迁移工具与默认生成。
- 日志与就绪：
  - 适配器实现 `ParseKernelLog/IsKernelReady/ExtractErrorMessage`，UI 侧据此提示启动/错误状态。

协议映射速览（Outbound 能力）：
- sing-box：`vmess`/`vless`/`ss`/`trojan`/`socks`/`http`/`wireguard`/`hysteria(2)`/`tuic`/`anylts`。
- mihomo：`vmess`/`vless`/`ss`/`trojan`/`socks5`/`http`/`hysteria(2)`/`tuic`/`wireguard`，Clash 规则/策略组。
- xray：`vmess`/`vless`/`ss`/`trojan`/`socks`/`http` + Reality。


## 可以优化的点（建议直接修改）

- 内核版本获取统一化：
  - 现部分适配器 `GetVersion()` 返回 "Unknown"，而 `PolyCoreKernelManager::RegisterKernel()` 已通过 `QueryKernelVersion()` 解析版本。建议：
    - 将版本号写入 `KernelInfo.version` 并可选持久化缓存；
    - 适配器内 `GetVersion()` 与管理器解析结果对齐，避免不一致。
- UI 偏好与运行态内核解耦：
  - `w_PreferencesWindow` 仍保留 V2Ray 命名的路径项，已新增“内核列表”视图但仍可加强：
    - 新增统一“内核管理”页：列出发现的内核（名称/路径/版本/能力），可设置首选与优先级；
    - 为不同内核设置独立统计端口/特性开关；
    - 连接编辑器可覆盖选择目标内核并提示兼容性。
- 配置校验增强：
  - mihomo `ValidateConfig()` 目前以正则检查 YAML 顶层键，建议：
    - 若内核提供测试开关则调用；
    - 否则引入轻量 YAML schema 校验，或 round-trip（生成→解析→再生成）一致性检查。
- 路由/规则互转健壮性：
  - `GenerateSingBoxRoute` 与 `GenerateClashRules/ProxyGroup` 覆盖常见场景，但对 `geosite/geoip` 组合、端口范围、`domain/keyword` 混用等边界需补充用例。
- Stats 对齐：
  - sing-box/mihomo 多通过 Clash API 获取统计，建议抽象统一统计采集层，UI 不感知内核差异。
- 自动发现路径补充：
  - 已含多数常见路径，建议加 `opt/homebrew/bin` 与自定义路径输入；Windows 支持相对路径（如 `./core/`）。
- 内核日志等级/编码：
  - 生成配置时明确日志等级/编码，避免多语言环境乱码与过量输出。

已发现的具体问题（应尽快修复）：
- 适配器校验命令未指定可执行路径：
  - `SingBoxKernelAdapter::ValidateConfig()` 与 `V2RayKernelAdapter::ValidateConfig()` 使用 `process.start("", args)`，会导致无法启动校验进程。应改为由 `PolyCoreKernelManager` 统一以已注册的内核路径调用，或为适配器提供 `SetExecutablePath()`。
- mihomo YAML 输出实现简化：
  - `MihomoKernelAdapter` 的 JSON→YAML 转换为手写流函数，存在缩进/引号/特殊字符风险；且源码中包含 `#include <QYamlDocument>`（Qt 并无该类）。建议改为使用成熟 YAML 库（如 `yaml-cpp`）或稳定转换器，移除伪依赖。
- 版本解析健壮性：
  - `ParseVersionFromOutput` 使用正则捕获，建议增加多样样本单测，避免内核输出变更导致回退为“首行全文”。
- UI 阻塞调用：
  - `PreferencesWindow::on_testApiBtn_clicked()` 使用阻塞 `QEventLoop`，可改异步并在按钮上显示 loading/状态提示。


## 建议的代码级改动（指引）

- 在 `PolyCoreKernelManager`：
  - `AutoDetectKernels()` 后将解析的版本写回 `availableKernels[type].version`；
  - 持久化 `preferredKernel`（已提供 `SetPreferredKernel*`），在设置加载时应用；
  - 统一执行“版本/校验”命令，避免适配器拿不到 exe 路径。
- 在 `SingBoxKernelAdapter`：
  - `GetVersion()`：执行 `<exe> version` 并解析 `sing-box version ...` 行；
  - `GenerateSingBoxDNS/Route`：对未知字段忽略并警告，防止生成非法配置。
- 在 `MihomoKernelAdapter`：
  - `GetVersion()`：执行 `<exe> -v` 并解析；
  - `ValidateConfig()`：引入 YAML 解析回读或 schema 校验，替换手写转换；
  - JSON→YAML：采用库序列化，删除自写 `Convert*ToYaml`。
- 在 `V2RayKernelAdapter`：
  - `ValidateConfig()` 同样不应 `start("")`；
  - Reality 相关：继续与 `ProtocolSupport` 数据模型对齐，避免字段漂移。
- 在 `ProtocolSupport`：
  - 将 Reality/Anylts 的 `Validate*` 对应到 UI 表单（必填与格式校验），降低启动失败率。
- UI：
  - 完善“内核管理”页，支持一键测试版本、设置首选、查看协议能力；
  - 连接编辑器增加“目标内核选择”与兼容性提示。

接口与实现解耦建议：
- 将适配器接口变更为 `ValidateConfig(configPath, exePath)` 或完全由管理器执行校验流程；
- 抽象 `IStatsProvider`，由适配器返回具体实现，管理器统一订阅并转发；
- 正则与性能：常用正则静态初始化或缓存，避免频繁编译。


## 风险与兼容性

- 配置互转差异：不同内核对同名字段语义不同，UI 需告知“不支持/语义差异”。
- Windows 沙箱/权限：子进程启动参数可能被拦截，增加重试/提示。
- geosite/geoip 资源：路径/格式不符需明确报错并给出修复指引。
- Android 组合限制：仅 Qt6 + QML，CMake 选项需防止错误组合导致构建失败。
- YAML 健壮性：错误 YAML 会导致 mihomo 静默失败，需在生成后立即解析回读并报错定位。


## 测试补齐（建议新增）

- 单元测试（`test/src/...`）：
  - 配置生成快照：为 sing-box/mihomo/v2ray/xray 构造典型 inbounds/outbounds/routing/DNS 输入，校验生成 JSON/YAML。
  - Reality/Anylts：合法/非法用例校验器（必填/格式/范围）。
  - 日志与就绪判定：常见启动成功/失败片段解析。
- 集成测试：
  - 启动“干运行/校验”模式（若有），验证端口监听与 API 可达；
  - 统计 API：对 Clash API 与 Xray stats 的对齐测试。
- 版本解析：
  - 为 `sing-box`/`mihomo`/`xray`/`v2ray` 构造多风格 `--version` 输出样本，断言解析结果。
- 路由规则映射：
  - `geosite:`/`geoip:`/`domain:`/`keyword:` 混合输入，比较 sing-box 规约与 Clash 规约差异是否按预期生成。
- YAML 健壮性：
  - 引号、冒号、非 ASCII 字符、多行字符串等边界验证。


## 目录导览（与关注文件）

- 内核管理：`src/core/kernel/PolyCoreKernelManager.{hpp,cpp}`
- 适配器：
  - sing-box：`src/core/kernel/SingBoxKernelAdapter.{hpp,cpp}`
  - mihomo：`src/core/kernel/MihomoKernelAdapter.{hpp,cpp}`
  - v2ray/xray：`src/core/kernel/V2RayKernelAdapter.{hpp,cpp}`
- 协议建模：`src/core/protocols/ProtocolSupport.{hpp,cpp}`
- DNS（sing-box）：`src/core/dns/SingBoxDNSConfig.{hpp,cpp}`
- UI（偏好/连接）：
  - `src/ui/widgets/windows/w_PreferencesWindow.{ui,hpp,cpp}`
  - `src/ui/widgets/windows/w_MainWindow.{ui,hpp,cpp}`
  - `src/ui/widgets/widgets/*`（连接/路由/DNS 编辑）
- 构建与打包：
  - 根构建：`CMakeLists.txt`
  - 组件与平台：`cmake/components/*.cmake`, `cmake/platforms/*.cmake`
  - 版本资源（Win/macOS）：`cmake/versioninfo/*`


## 验收清单（落地效果）

- 设置界面展示 sing-box/mihomo 的路径、版本、能力状态，并可一键测试与设为首选。
- 连接编辑器可选择目标内核；若协议与内核不兼容立即提示。
- 启动日志明确显示所选内核、版本、关键特性（Reality/Anylts）。
- 统计面板在不同内核下显示一致指标（下行/上行/连接数等）。
- 单测覆盖关键行为，CI 在三平台构建与测试通过。


## 参考备注

- 构建指南：`BUILD_GUIDE.md` 对多内核文件/自动检测/运行目录有清楚说明。


## CMake / 构建系统可优化项

- Qt 自动探测：当前通过 `QV2RAY_QT6` 与 UI 类型推导，可增加 `find_package(Qt6 ...)`/`find_package(Qt5 ...)` 失败回退与清晰错误消息。
- 测试矩阵：根 `CMakeLists.txt` 已支持 `BUILD_TESTING`，建议 CI 启用并执行 `ctest`。
- RPATH/部署：为 macOS/Linux 设置合理 RPATH 与 `qt.conf`；Windows 集成 `windeployqt` 目标。
- 选项命名：逐步将 `QV2RAY_*` 迁移为 `POLYCORE_*`，并兼容旧名（`if(DEFINED OLD) set(NEW ${OLD}) endif()`）。


## 代码质量与一致性

- 命名迁移：参考 `docs/RENAMING_STRATEGY.md`，逐步将 `Qv2ray` 前缀替换为 `PolyCore`，同步宏（如 `QV2RAY_GUI` → `POLYCORE_GUI`）。
- YAML 依赖：移除 `#include <QYamlDocument>`，采用 `yaml-cpp` 或稳定 JSON↔YAML 转换工具；避免手写序列化。
- 适配器职责：适配器专注“配置生成/日志解析/能力声明”，由管理器负责“路径/进程/版本探测/校验调用”。
- I/O 安全：临时文件创建后确保权限合理，异常路径及时清理（当前 `CleanupConnection()` 已移除临时配置）。
- 非阻塞 UI：网络/API 检查走异步链路，UI 上显示进度与结果文本。


## 安全与健壮性

- 输入校验：对 Reality/Anylts 字段在 UI 提交前进行本地校验（空值/格式/范围）。
- 进程管理：`terminate()` 超时后 `kill()` 的流程增加平台提示与告警。
- 日志脱敏：配置/日志输出避免密码/密钥/uuid 明文，必要时脱敏（显示前/后几位）。
- 配置路径：Windows 注意 UAC 与 Program Files 写入权限，默认推荐用户目录。


## 进一步工作路线（Roadmap）

- 统一统计 API：抽象 `IStatsProvider` 并实现 sing-box/mihomo/xray 查询器，UI 统一渲染。
- 完成重命名计划：代码/宏/资源/翻译/桌面文件名统一（兼容旧配置与路径）。
- 引入 schema 校验：对 JSON/YAML 生成后进行 schema 校验与 round-trip 解析，提升稳定性。
- 完善文档：在 `BUILD_GUIDE.md` 增补 Qt 环境变量设置示例（Windows/macOS/Linux），在 README 中补充 PolyCore 分支与多内核能力概览。


## Windows 构建错误修复方案（Protobuf/gRPC 与构建目录问题）

你可能会遇到如下 CMake 报错：

- “Could NOT find Protobuf (missing: Protobuf_LIBRARIES Protobuf_INCLUDE_DIR) (found version "29.3.0")”
- “The command cmake.exe ... terminated with exit code 1.”（由上条错误引发）
- “No CMake configuration for build type "Debug" found.”（生成器与构建类型不匹配）

根因：只发现了 `protoc` 可执行文件，但缺少 Protobuf 开发库/头文件；同时把构建目录错误地指到了源码的 `cmake/` 目录，导致缓存/配置混乱。

### 一次到位（推荐）— vcpkg 安装依赖 + 正确的 out-of-source 构建
1) 清理错误构建缓存
- 关闭 IDE/Qt Creator。
- 删除源码里误生成的缓存：`Qv2ray-dev/cmake/CMakeCache.txt` 与 `Qv2ray-dev/cmake/CMakeFiles/`（注意是源码下 `cmake/` 目录）。
- 新建干净构建目录，例如 `build-msvc`。

2) 用 vcpkg 安装依赖（自动提供 Protobuf/gRPC 的 CMake 包）
```bat
vcpkg install grpc:x64-windows
```
说明：安装 gRPC 会自动安装 `protobuf`、`abseil`、`re2`、`c-ares`、`openssl`、`zlib` 等依赖。

3) 把工具加入 PATH（让 CMake 找到 grpc_cpp_plugin/protoc）
- 加入（替换成你的 vcpkg 路径）：
  - `D:\vcpkg\installed\x64-windows\tools\grpc`
  - `D:\vcpkg\installed\x64-windows\tools\protobuf`

4) 正确配置与生成（MSVC 多配置生成器）
```bat
cd D:\Cursor-Pro\PolyCore\Qv2ray-dev\Qv2ray-dev
cmake -S . -B build-msvc ^
  -G "Visual Studio 17 2022" -A x64 ^
  -DQV2RAY_UI_TYPE=QWidget ^
  -DCMAKE_TOOLCHAIN_FILE=D:/vcpkg/scripts/buildsystems/vcpkg.cmake ^
  -DVCPKG_TARGET_TRIPLET=x64-windows
cmake --build build-msvc --config Release -j
```
说明：
- 使用 VS 生成器即可同时拥有 Debug/Release，不会再出现 “No CMake configuration for build type Debug found”。
- 指定 vcpkg 工具链后，`find_package(Protobuf/gRPC)` 能自动找到库与头文件。

5) 若使用 Qt Creator
- CMake 选项中加入：
  - `-DCMAKE_TOOLCHAIN_FILE=D:/vcpkg/scripts/buildsystems/vcpkg.cmake`
  - `-DVCPKG_TARGET_TRIPLET=x64-windows`
- 构建目录不要指向源码的 `cmake/` 子目录，使用独立目录（如 `build-msvc`）。
- 生成器选择 MSVC（多配置）。

### 方案 B — 已手动安装 gRPC/Protobuf（不用 vcpkg）
若已从源码/安装包安装：
```bat
cmake -S . -B build-msvc ^
  -G "Visual Studio 17 2022" -A x64 ^
  -DProtobuf_DIR=C:/thirdparty/protobuf/cmake ^
  -DgRPC_DIR=C:/thirdparty/grpc/lib/cmake/grpc
```
并将 `protoc.exe` 与 `grpc_cpp_plugin.exe` 所在目录加入 PATH，或传 `-DGRPC_CPP_PLUGIN=C:/thirdparty/grpc/bin/grpc_cpp_plugin.exe`。

提示：只有 `protoc.exe` 不够，`FindProtobuf` 还需要 `Protobuf_INCLUDE_DIR` 与 `Protobuf_LIBRARIES`。

### 方案 C — 临时规避依赖（不推荐，需要改 CMake）
如短期无法安装依赖，可新增开关跳过 `cmake/backend.cmake`（禁用相关功能）：
```cmake
option(QV2RAY_ENABLE_GRPC_BACKEND "Enable gRPC/protobuf backend" ON)
if(QV2RAY_ENABLE_GRPC_BACKEND)
  include(cmake/backend.cmake)
endif()
```
并确保 `target_link_libraries` 对 `${QV2RAY_BACKEND_LIBRARY}` 做条件处理。慎用。


## Qt Creator 使用方式（多方案）

### 方式一（推荐）：MSVC Kit + vcpkg 工具链 + 多配置生成器
- 选择 Kit：Qt 5.15/6.x MSVC 64-bit。
- Build Settings → CMake → 添加变量：
  - `CMAKE_TOOLCHAIN_FILE = D:/vcpkg/scripts/buildsystems/vcpkg.cmake`
  - `VCPKG_TARGET_TRIPLET = x64-windows`
  - `QV2RAY_UI_TYPE = QWidget`
- Build Environment：将以下目录加入 `PATH`（替换为你的 vcpkg 路径）：
  - `D:/vcpkg/installed/x64-windows/tools/grpc`
  - `D:/vcpkg/installed/x64-windows/tools/protobuf`
- Build Directory：独立目录（例如 `build-msvc`），不要指向源码的 `cmake/` 子目录。
- 说明：使用 MSVC + Ninja Multi-Config（Qt Creator 默认）或 Visual Studio 生成器，Debug/Release 均可，无需设置 `CMAKE_BUILD_TYPE`。

### 方式二：MSVC Kit（不用 vcpkg），手动指向已安装的 gRPC/Protobuf
- Build Settings → CMake → 添加变量：
  - `Protobuf_DIR = C:/thirdparty/protobuf/cmake`
  - `gRPC_DIR = C:/thirdparty/grpc/lib/cmake/grpc`
- Build Environment：把 `protoc.exe`、`grpc_cpp_plugin.exe` 所在目录加入 `PATH`，或设置 `GRPC_CPP_PLUGIN` 指向可执行文件。
- 说明：必须具备 `Protobuf_INCLUDE_DIR` 与 `Protobuf_LIBRARIES`，仅 `protoc` 不够。

### 方式三：Ninja 单配置 Kit（只有单配置生成器时）
- Build Settings → CMake → 添加：
  - `CMAKE_BUILD_TYPE = Debug`（或 `Release`）
- 其他同方式一（vcpkg 工具链）。
- 说明：单配置生成器必须显式指定 `CMAKE_BUILD_TYPE`，避免 Debug 配置报错。

### 方式四：MinGW Kit（仅在不需要 gRPC/Protobuf 后端时）
- Build Settings → CMake → 添加：
  - `QV2RAY_ENABLE_GRPC_BACKEND = OFF`
- 说明：gRPC 在 Windows + MinGW 组合支持较弱，建议禁用后端功能再编译；否则切换至 MSVC Kit。

### 方式五（临时规避，不推荐长期使用）：关闭后端
- Build Settings → CMake → 添加：
  - `QV2RAY_ENABLE_GRPC_BACKEND = OFF`
- 说明：会禁用依赖 gRPC/Protobuf 的功能，仅用于先通过构建。

### 通用注意事项
- 构建目录务必独立（如 `build-qtcreator-msvc`），避免使用源码 `cmake/` 目录。
- 修改 Kit/变量后点击“重新配置”，再开始编译。
- Qt 6 + QML 使用 Qt6 Kit；Widgets 默认均可，用 Qt5/Qt6 取决于环境。

#### PowerShell 快速接入 Qt Creator（只做到依赖安装第 2 步即可）
在 PowerShell 中完成 vcpkg 初始化与依赖安装（到第 2 步）：
```powershell
git clone https://github.com/microsoft/vcpkg.git D:\vcpkg
cd D:\vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg install grpc:x64-windows
```
然后直接在 Qt Creator 中配置：
- 选 Kit：Qt 6.9 MSVC 64-bit
- Build Settings → CMake 变量：
  - `CMAKE_TOOLCHAIN_FILE = D:/vcpkg/scripts/buildsystems/vcpkg.cmake`
  - `VCPKG_TARGET_TRIPLET = x64-windows`
  - `QV2RAY_QT6 = ON`
  - `QV2RAY_UI_TYPE = QWidget`（或 `QML`）
- Build Environment：将以下目录加入 PATH（让 Qt Creator 找到 protoc/grpc_cpp_plugin）：
  - `D:/vcpkg/installed/x64-windows/tools/grpc`
  - `D:/vcpkg/installed/x64-windows/tools/protobuf`
- Build Directory：独立目录（如 `build-qt6`），点击“Reconfigure”后再构建。

## Qt 6.9 构建说明

- 兼容性：项目已支持 Qt6，使用 Qt 6.9 可以直接构建。
- 需求：CMake ≥ 3.16（已满足），推荐 MSVC 64-bit Kit；如需 gRPC/Protobuf，按上文 vcpkg 方案安装依赖。

命令行构建（Qt Widgets 示例）：
```bat
cmake -S . -B build-qt6 ^
  -G "Visual Studio 17 2022" -A x64 ^
  -DQV2RAY_QT6=ON ^
  -DQV2RAY_UI_TYPE=QWidget ^
  -DCMAKE_PREFIX_PATH=C:/Qt/6.9.0/msvc2019_64
cmake --build build-qt6 --config Release -j
```
如使用 vcpkg，再追加：
```bat
  -DCMAKE_TOOLCHAIN_FILE=D:/vcpkg/scripts/buildsystems/vcpkg.cmake ^
  -DVCPKG_TARGET_TRIPLET=x64-windows
```
并将 `D:/vcpkg/installed/x64-windows/tools/grpc` 与 `.../tools/protobuf` 加入 PATH。

Qt Creator（推荐）：
- 选 Kit：Qt 6.9 MSVC 64-bit
- Build Settings → CMake 变量：
  - `QV2RAY_QT6=ON`
  - `QV2RAY_UI_TYPE=QWidget`（或 `QML`）
  - 如用 vcpkg：再加 `CMAKE_TOOLCHAIN_FILE` 与 `VCPKG_TARGET_TRIPLET`
- Build Environment：把 `.../tools/grpc`、`.../tools/protobuf` 加入 PATH
- Build Directory：独立目录（如 `build-qt6`）

说明：
- 选择 `QML` 界面时，项目会自动启用 Qt6。
- 若暂未安装 gRPC/Protobuf，可临时关闭后端（`QV2RAY_ENABLE_GRPC_BACKEND=OFF`）先通过构建（不建议长期）。

