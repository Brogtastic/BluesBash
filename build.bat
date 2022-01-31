@echo off
rem alway start with the correct working directory
cd %~dp0

call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"

rem ensure output directories do exist.
if not exist bin mkdir bin
if not exist bin\obj mkdir bin\obj
if not exist libs (
	 echo The .\libs\ folder is missing, make sure to supply it with raylib header and library files.
	 exit
)

set IncludePaths=-I./libs/raylib/include
set LibraryPaths=/LIBPATH:./libs/raylib/lib
set TranslationUnits=source/BluesBash.cpp

set CFlags=-nologo -Fo./bin/obj/ -MD
set LFlags=-link /OUT:./bin/BluesBash.exe /nologo


@echo on
cl %CFlags% %IncludePaths% %TranslationUnits% %LFlags% %LibraryPaths%
@echo off
