# Qv2ray 多内核版本编译指南

## 概述

我们已经成功将 sing-box 和 mihomo 内核集成到 Qv2ray 中。源码修改已完成，现在需要编译项目。

## 已完成的集成工作

### ✅ 内核文件准备
- **sing-box v1.12.0** (36MB) - 支持全功能协议栈
- **mihomo alpha-e89af72** (30MB) - Clash Meta 兼容

### ✅ 源码修改
1. **PolyCoreKernelManager.cpp:319** - 添加 `mihomo-windows-amd64.exe` 到自动检测
2. **MihomoKernelAdapter.cpp** - 完整的 Clash/mihomo 适配器
3. **SingBoxKernelAdapter.cpp** - 完整的 sing-box 适配器

### ✅ 功能验证
- 两个内核都能正常响应版本查询
- 自动检测代码已更新
- 多内核管理系统完整

## 编译环境要求

### Windows 环境
```bash
# 需要的工具
- Visual Studio 2019/2022 (含 C++ 构建工具)
- Qt 5.15+ 或 Qt 6.x
- CMake 3.16+
- Git
```

### 快速编译步骤

#### 1. 设置环境
```cmd
# 启动 Visual Studio Developer Command Prompt
# 或设置环境变量
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat"

# 设置 Qt 环境
set PATH=C:\Qt\6.5.0\mingw_64\bin;%PATH%
set CMAKE_PREFIX_PATH=C:\Qt\6.5.0\mingw_64
```

#### 2. 配置构建
```bash
mkdir build
cd build

# 使用 Qt Widget 界面 (推荐)
cmake .. -DQV2RAY_UI_TYPE=QWidget -DCMAKE_BUILD_TYPE=Release

# 或使用 QML 界面
cmake .. -DQV2RAY_UI_TYPE=QML -DCMAKE_BUILD_TYPE=Release
```

#### 3. 编译项目
```bash
# 使用 CMake 构建
cmake --build . --config Release -j4

# 或使用 MSBuild (如果使用 Visual Studio)
msbuild Qv2ray.sln /p:Configuration=Release /m
```

#### 4. 部署运行
```bash
# 将以下文件复制到输出目录
cp ../sing-box.exe ./Release/
cp ../mihomo-windows-amd64.exe ./Release/

# 运行程序
cd Release
./Qv2ray.exe
```

## 预期功能

### 🎯 多内核选择
- 启动时自动检测 sing-box 和 mihomo 内核
- 在连接配置中可选择使用的内核类型
- 支持热切换不同内核

### 🎯 协议支持
**sing-box 内核支持:**
- VMess, VLESS, Shadowsocks, Trojan
- Hysteria, Hysteria2, TUIC
- WireGuard, SSH, ANYLTS
- Reality, uTLS 等高级功能

**mihomo 内核支持:**
- 完整 Clash 配置兼容
- VMess, VLESS, Shadowsocks, Trojan
- Hysteria, TUIC, WireGuard
- Clash 规则和策略组

### 🎯 高级功能
- 智能路由和分流
- 实时流量统计
- 规则编辑器
- 订阅管理
- 插件系统

## 故障排除

### 编译失败
1. 检查 Qt 和编译器环境变量
2. 确认 CMake 能找到所有依赖
3. 查看具体错误信息调整配置

### 内核检测失败
1. 确认内核文件在 Qv2ray.exe 同目录
2. 检查文件执行权限
3. 测试内核版本命令是否正常

### 连接问题
1. 检查内核选择是否正确
2. 验证配置格式兼容性
3. 查看内核日志输出

## 简化方案

如果编译环境配置困难，可以考虑：

1. **使用现有预编译版本** + 替换内核文件
2. **在 Linux/Docker 环境**中编译
3. **使用 GitHub Actions** 自动构建

## 项目结构

编译完成后的目录结构：
```
Qv2ray-Release/
├── Qv2ray.exe              # 主程序
├── sing-box.exe             # sing-box 内核
├── mihomo-windows-amd64.exe # mihomo 内核
├── Qt6*.dll                 # Qt 运行库
├── platforms/               # Qt 平台插件
└── config/                  # 配置目录
```

编译成功后，Qv2ray 将自动检测并支持两个内核，实现真正的多内核代理功能！