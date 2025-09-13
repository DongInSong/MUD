// 2_build.bat
@echo off
if exist build cd build && cmake --build . && cd ..
echo "Build complete."
exit /b 0