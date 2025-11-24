# Visual Studio Build Instructions

1. **Setup vcpkg** (one-time):
   ```powershell
   git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
   C:\vcpkg\bootstrap-vcpkg.bat
   C:\vcpkg\vcpkg.exe integrate install
   ```

2. **Build in Visual Studio:**
   - Open the Vira folder in Visual Studio
   - Wait for CMake configuration to complete
   - Build -> Build All