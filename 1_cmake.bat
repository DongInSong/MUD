@echo off
if exist build rmdir /s /q build
mkdir build
echo "Creating build directory..."
cd build
echo "Configuring project with CMake..."
@REM cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake ..
echo "Building project..."
cmake --build . 
cd ..
echo "Build complete."
exit /b 0