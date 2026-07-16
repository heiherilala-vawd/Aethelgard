@echo off
setlocal
set ENGINE_DIR=C:\Program Files\Epic Games\UE_5.8
set PROJECT_DIR=C:\Users\herilala\Documents\Unreal Projects\Aethelgard
set PROJECT=%PROJECT_DIR%\Aethelgard.uproject

call "%ENGINE_DIR%\Engine\Build\BatchFiles\Build.bat" AethelgardEditor Development Win64 "%PROJECT%" -NoEngineChanges -Log="%PROJECT_DIR%\Build.log"
echo Build exit code: %ERRORLEVEL%
pause
