# PolyCore Project Renaming Strategy

## Overview
Migration from Qv2ray to PolyCore - comprehensive renaming plan for multi-kernel support.

## Phase 1: Core Structure Renaming

### Namespaces
```cpp
// Old -> New
Qv2ray                          -> PolyCore
Qv2ray::core                   -> PolyCore::core
Qv2ray::core::kernel           -> PolyCore::core::kernel
Qv2ray::ui                     -> PolyCore::ui
Qv2ray::components             -> PolyCore::components
```

### Classes and Interfaces
```cpp
// Application Core
Qv2rayApplicationInterface     -> PolyCoreApplicationInterface
Qv2rayBaseApplication          -> PolyCoreBaseApplication
QvCoreApplication              -> PolyCoreApplication

// Kernel Management  
V2RayKernelInstance           -> PolyCoreKernelInstance (deprecated, use PolyCoreKernelManager)
QvKernelABIChecker            -> PolyCoreKernelABIChecker
KernelInstanceHandler         -> PolyCoreKernelHandler

// Configuration
QvConfigHandler               -> PolyCoreConfigHandler
Qv2rayConfigObject           -> PolyCoreConfigObject
QvComplexConfigModels        -> PolyCoreComplexConfigModels
QvCoreSettings               -> PolyCoreCoreSettings
QvSettingsObject             -> PolyCoreSettingsObject
QvStartupConfig              -> PolyCoreStartupConfig

// UI Components
MainWindow                    -> PolyCoreMainWindow (keep as MainWindow for UI)
QvMessageBus                 -> PolyCoreMessageBus
QvAutoCompleteTextEdit       -> PolyCoreAutoCompleteTextEdit

// Plugin System
Qv2rayInterface              -> PolyCoreInterface  
QvPluginHost                 -> PolyCorePluginHost
QvPluginMetadata             -> PolyCorePluginMetadata

// Utilities
QvHelpers                    -> PolyCoreHelpers
QvTranslator                 -> PolyCoreTranslator
QvNTPClient                  -> PolyCoreNTPClient
```

### File/Directory Structure
```
src/base/Qv2rayBase.hpp                -> src/base/PolyCoreBase.hpp
src/base/Qv2rayBaseApplication.*       -> src/base/PolyCoreBaseApplication.*
src/base/Qv2rayFeatures.hpp           -> src/base/PolyCoreFeatures.hpp
src/base/Qv2rayLog.hpp                 -> src/base/PolyCoreLog.hpp
src/base/models/Qv*                    -> src/base/models/PolyCore*

src/core/kernel/V2RayKernelInteractions.* -> src/core/kernel/PolyCoreKernelInteractions.*
src/core/kernel/QvKernelABIChecker.*      -> src/core/kernel/PolyCoreKernelABIChecker.*

src/ui/Qv2rayPlatformApplication.*     -> src/ui/PolyCorePlatformApplication.*
src/ui/cli/Qv2rayCliApplication.*      -> src/ui/cli/PolyCoreCliApplication.*
src/ui/qml/Qv2rayQMLApplication.*      -> src/ui/qml/PolyCoreQMLApplication.*
src/ui/widgets/Qv2rayWidgetApplication.* -> src/ui/widgets/PolyCoreWidgetApplication.*

src/components/plugins/QvPluginHost.* -> src/components/plugins/PolyCorePluginHost.*
src/components/translations/QvTranslator.* -> src/components/translations/PolyCoreTranslator.*
src/utils/QvHelpers.*                 -> src/utils/PolyCoreHelpers.*
```

### CMake Variables and Macros
```cmake
# Old -> New
QV2RAY_*                      -> POLYCORE_*
qv2ray                        -> polycore  
Qv2ray                        -> PolyCore

# Examples:
QV2RAY_VERSION_STRING         -> POLYCORE_VERSION_STRING
QV2RAY_UI_TYPE               -> POLYCORE_UI_TYPE
QV2RAY_HAS_BUILTIN_PLUGINS   -> POLYCORE_HAS_BUILTIN_PLUGINS
QV2RAY_USE_V5_CORE           -> POLYCORE_USE_V5_CORE
QV2RAY_QT6                   -> POLYCORE_QT6
```

### Global Macros and Defines
```cpp
// Old -> New
#define GlobalConfig             -> #define GlobalPolyCoreConfig
QV2RAY_VERSION_MAJOR            -> POLYCORE_VERSION_MAJOR
QV2RAY_VERSION_MINOR            -> POLYCORE_VERSION_MINOR
QV2RAY_VERSION_BUILD            -> POLYCORE_VERSION_BUILD
QV2RAY_GUI                      -> POLYCORE_GUI
QV2RAY_CLI                      -> POLYCORE_CLI
_QV2RAY_BUILD_INFO_STR_         -> _POLYCORE_BUILD_INFO_STR_
_QV2RAY_BUILD_EXTRA_INFO_STR_   -> _POLYCORE_BUILD_EXTRA_INFO_STR_
```

## Phase 2: String Constants and Resources

### Application Info
```cpp
// Application metadata
"Qv2ray"                      -> "PolyCore"  
"qv2ray"                      -> "polycore"
"Qv2ray Workgroup"            -> "PolyCore Team"
"qv2ray-config-models"        -> "polycore-config-models"
"qv2ray.desktop"              -> "polycore.desktop"
"qv2ray.metainfo.xml"         -> "polycore.metainfo.xml"
```

### Registry/Settings Keys
```cpp
// Windows Registry / Settings
"Software/Qv2ray"             -> "Software/PolyCore"
"qv2ray"                      -> "polycore"
"Qv2ray"                      -> "PolyCore"
```

### Resource Files
```
assets/icons/qv2ray.*         -> assets/icons/polycore.*
assets/qv2ray.desktop         -> assets/polycore.desktop
assets/qv2ray.metainfo.xml    -> assets/polycore.metainfo.xml
translations/qv2ray_*.ts      -> translations/polycore_*.ts
```

## Phase 3: Implementation Priority

### High Priority (Core Functionality)
1. Update base application classes and namespaces
2. Rename kernel management classes
3. Update configuration system
4. Update plugin interfaces

### Medium Priority (UI Integration)
1. Update UI application classes
2. Update message bus and communication
3. Update settings and preferences
4. Update build system and CMake

### Low Priority (Polish)
1. Update resource files and assets
2. Update translations
3. Update documentation strings
4. Update about dialog and version info

## Phase 4: Migration Script Strategy

Create automated migration scripts for:
1. File renaming operations
2. Content replacement (class names, namespaces)  
3. CMake variable updates
4. Resource file updates
5. Build system updates

## Compatibility Considerations

1. **Settings Migration**: Provide migration path for existing user settings
2. **Plugin Compatibility**: Maintain backward compatibility where possible
3. **Build System**: Update all build scripts and deployment
4. **Documentation**: Update all references in docs and help files