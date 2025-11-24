# Vira Scratch Directory

If you wish to quickly put together test scripts, but not have them tracked by this repository, simply:
1. Create a subdirectory here
2. Add your own `CMakeLists.txt` and whatever source files you'd like to that project
3. Make sure that the CMake Option `VIRA_BUILD_SCRATCH` is `ON` (it defaults to `ON`)

The `CMakeLists.txt` should not be for a whole new project, as it will be built as part of the vira project.  For example:

```
add_executable(descent_simulation descent_simulation.cpp)
target_link_libraries(descent_simulation PRIVATE vira)
```