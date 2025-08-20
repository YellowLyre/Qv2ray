# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Qv2ray is a cross-platform V2Ray Qt GUI client written in C++ using Qt framework. It supports Windows, Linux, macOS, and Android platforms with a plugin system supporting various proxy protocols.

**Note**: This project is no longer maintained as of August 17, 2021 (v2.7.0 was the final release).

## Build System & Commands

### CMake Configuration
The project uses CMake as the build system with modular configuration files in `cmake/` directory.

**Build the project:**
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

**Build with specific UI type:**
- QWidget UI (default): `cmake .. -DQV2RAY_UI_TYPE=QWidget`
- QML UI: `cmake .. -DQV2RAY_UI_TYPE=QML`
- CLI only: `cmake .. -DQV2RAY_UI_TYPE=CLI`

**Testing:**
```bash
cmake .. -DBUILD_TESTING=ON
make -j$(nproc)
ctest
```

**Key CMake options:**
- `QV2RAY_AUTO_DEPLOY`: Auto-deploy after build (default: ON)
- `QV2RAY_HAS_BUILTIN_PLUGINS`: Build with builtin plugins (default: ON)
- `QV2RAY_QT6`: Use Qt6 instead of Qt5 (default: OFF, except for QML/Android)
- `QV2RAY_USE_V5_CORE`: Use V2Ray Core V5 (default: ON)

## Architecture Overview

### Core Components

**Base Layer (`src/base/`)**
- `Qv2rayBaseApplication`: Core application interface and startup argument handling
- Configuration models in `models/`: Core data structures and settings management
- Logging system and feature flags

**Core Layer (`src/core/`)**
- `CoreUtils`: Connection management utilities and protocol information
- `connection/`: Connection serialization/deserialization and config generation
- `handler/`: Config, kernel instance, and route handlers
- `kernel/`: V2Ray kernel interactions and API backend
- `settings/`: Settings persistence and upgrade logic

**UI Layer (`src/ui/`)**
- Three UI variants: QWidget (`widgets/`), QML (`qml/`), and CLI (`cli/`)
- Common UI utilities in `common/`: logging, QR codes, message bus, dark mode detection
- Widget-based UI includes main window, editors, node system, and various dialogs

**Plugin System**
- Built-in plugins for protocol support, subscription adapters, and utilities
- Plugin interface in `src/plugin-interface/`
- Extensible architecture for external plugins

### Key Architectural Patterns

**Configuration Management**
- JSON-based configuration with QJsonStruct for serialization
- Hierarchical config objects with validation
- Connection and group management with unique identifiers

**Protocol Support**
- Abstract outbound/inbound handlers
- Built-in support for VMess, VLESS, Shadowsocks, SOCKS, HTTP
- Plugin-based extension for additional protocols

**Node Editor System**
- Visual connection graph editor using QNodeEditor
- Support for complex routing configurations
- Rule-based traffic routing and balancing

## Development Guidelines

### Code Style (from CONTRIBUTING.md)
- Run `clang-format` before submitting patches
- Function/class names: UpperCamelCase
- Variables: camelCase  
- Namespaces: lowercase (except `::Qv2ray`)
- Use `.hpp` suffix for headers
- Forward declarations preferred over includes in headers
- Unused variables wrapped with `Q_UNUSED`

### Project Structure
```
src/
├── base/           # Core application foundation
├── core/           # Business logic and V2Ray integration  
├── ui/             # User interface layers (widget/qml/cli)
├── components/     # Utility components (latency, geosite, etc.)
├── plugins/        # Built-in plugin implementations
└── utils/          # Helper utilities

cmake/              # CMake configuration modules
3rdparty/           # Third-party dependencies
test/               # Unit tests using Catch2
assets/             # Application resources and icons
```

### Testing
Tests are located in `test/` directory using Catch2 framework:
- Connection parsing tests
- Configuration generation tests  
- Latency measurement tests
- JSON I/O tests

### Dependencies
Key third-party libraries integrated:
- Qt5/Qt6 (Core, Widgets/QML, Network, SVG)
- libuv for asynchronous I/O
- libcurl for HTTP operations
- QJsonStruct for JSON serialization
- QNodeEditor for visual node editing
- SingleApplication for instance management

## Platform-Specific Notes

**Windows**: Uses MSVC or MinGW, includes version resource generation
**Linux**: Standard Unix installation paths, desktop integration
**macOS**: Bundle creation with DMG packaging
**Android**: Qt6 required, QML UI only, special manifest handling