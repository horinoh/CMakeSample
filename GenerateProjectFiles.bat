@rem set CMAKE="C:\Program Files\CMake\bin\cmake"
@set CMAKE=cmake

@%CMAKE% --version

@rem CMAKE_GENERATOR_PLATFORM ... 64bit環境
@%CMAKE% -DCMAKE_GENERATOR_PLATFORM=x64
@rem %CMAKE% -G "Visual Studio 14 2015 Win64"

@pause

@start MyProject.sln