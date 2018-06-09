@rem set CMAKE="C:\Program Files\CMake\bin\cmake"
@set CMAKE=cmake

@%CMAKE% --version

del CMakeCache.txt 

@%CMAKE% -DCMAKE_GENERATOR_PLATFORM=x64

@pause

@start MyProject.sln