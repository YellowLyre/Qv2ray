@echo off
echo PolyCore - 多核心代理客户端启动器
echo.

:menu
echo 请选择要启动的核心:
echo 1. sing-box (全功能核心)
echo 2. Mihomo (Clash Meta 兼容核心)
echo 3. 显示核心版本信息
echo 4. 退出
echo.
set /p choice=请输入选择 (1-4): 

if "%choice%"=="1" goto singbox
if "%choice%"=="2" goto mihomo
if "%choice%"=="3" goto version
if "%choice%"=="4" goto exit
echo 无效选择，请重试
goto menu

:singbox
echo.
echo 启动 sing-box...
echo 使用 Ctrl+C 停止
echo.
sing-box.exe run
goto menu

:mihomo
echo.
echo 启动 Mihomo...
echo 使用 Ctrl+C 停止
echo.
mihomo-windows-amd64.exe
goto menu

:version
echo.
echo === 核心版本信息 ===
echo.
echo sing-box 版本:
sing-box.exe version
echo.
echo Mihomo 版本:
mihomo-windows-amd64.exe -v
echo.
pause
goto menu

:exit
echo.
echo 感谢使用 PolyCore！
pause