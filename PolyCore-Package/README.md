# PolyCore - 多核心代理客户端

这是一个集成了多个代理核心的完整包，基于Qv2ray项目。

## 包含的核心

### 1. sing-box v1.12.0
- **功能特性**: 支持 gVisor, QUIC, DHCP, WireGuard, uTLS, ACME, Clash API, Tailscale
- **环境信息**: Go 1.24.5 Windows/AMD64
- **标签**: with_gvisor,with_quic,with_dhcp,with_wireguard,with_utls,with_acme,with_clash_api,with_tailscale
- **版本信息**: Revision 2dcb86941f5db01bbe296acae555a3b1f38c0441

### 2. Mihomo Meta Alpha
- **版本**: alpha-e89af72 (2025年8月1日构建)
- **功能特性**: 支持 gVisor
- **环境信息**: Go 1.24.5 Windows AMD64

## 核心能力对比

| 功能 | sing-box | Mihomo |
|------|----------|--------|
| Clash API | ✅ | ✅ |
| gVisor | ✅ | ✅ |
| QUIC | ✅ | ❌ |
| WireGuard | ✅ | ❌ |
| uTLS | ✅ | ❌ |
| Tailscale | ✅ | ❌ |
| DHCP | ✅ | ❌ |
| ACME | ✅ | ❌ |

## 使用说明

1. **sing-box**: 适合需要全功能支持的高级用户
2. **mihomo**: 适合需要 Clash 兼容性的用户

## 配置建议

- 如需最新功能和协议支持，推荐使用 sing-box
- 如需 Clash Meta 兼容性，推荐使用 Mihomo
- 两个核心都支持现代代理协议和规则路由

## 技术信息

- 构建时间: 2025年8月6日
- 平台支持: Windows x64
- 编译器: Go 1.24.5