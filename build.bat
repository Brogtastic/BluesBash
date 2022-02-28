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

set IncludePaths=-I../libs/raylib/include
set LibraryPaths=/LIBPATH:../libs/raylib/lib
set TranslationUnits=BluesBash.cpp

set CFlags=-nologo -Fo../bin/obj/
set LFlags=-link /OUT:../bin/BluesBash.exe /nologo

if "%1"=="debug" (
	 set CFlags=%CFlags% -Zi -Od -MD
	 set LFlags=%LFlags% /DEBUG
) else (
	 set CFlags=%CFlags% -O2 -MT
	 set LFlags=%LFlags% 
)

pushd source
@echo on
cl %CFlags% %IncludePaths% %TranslationUnits% %LFlags% %LibraryPaths%
@echo off
popd
