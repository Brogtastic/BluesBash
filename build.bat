@echo off
rem alway start with the correct working directory
cd %~dp0

call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"

rem ensure output directories do exist.
if not exist bin mkdir bin
if not exist bin\obj mkdir bin\obj
if not exist libs (
	 echo The .\libs\ folder is missing, make sure to supply it with raylib header and library files and the Odin compiler.
	 exit
)

set IncludePaths=-Ilibs\raylib\include
set LibraryPaths=/LIBPATH:libs\raylib\lib\
set TranslationUnits=source\BluesBash.cpp

set CFlags=-nologo -Fobin\obj\
set LFlags=-link /OUT:bin\BluesBash.exe /nologo

if "%1"=="debug" (
	 set CFlags=%CFlags% -Zi -Od -MD
	 set LFlags=%LFlags% /DEBUG
) else (
	 set CFlags=%CFlags% -O2 -MT
	 set LFlags=%LFlags% 
)

echo Building Tools...

set OdinC="libs/Odin/odin.exe"
@echo on 
%OdinC% build source\Tools\PicturePreProcessor.odin -out:bin\PicturePreProcessor.exe
@echo off

echo Cleaning bin\resources\processed...
if not exist bin\resources\processed mkdir bin\resources\processed
del /Q bin\resources\processed\* 

echo Running Tools...

pushd bin
PicturePreProcessor.exe resources\animations\intro\Intro%%04d.png 266 1 24 Intro.ppp
PicturePreProcessor.exe resources\animations\play\play%%d.png 5 1 15 PlayButton.ppp
PicturePreProcessor.exe resources\animations\settings\settings%%d.png 5 1 60 SettingsButton.ppp
PicturePreProcessor.exe resources\animations\listen\listen%%d.png 5 1 30 ListenButton.ppp
PicturePreProcessor.exe "resources\animations\login page\login%%d.png" 28 1 15 LoginButton.ppp
PicturePreProcessor.exe resources\animations\light\Light%%d.png 13 1 15 TopMenuLight.ppp
PicturePreProcessor.exe "resources\gameplay screen.png" 1 0 1 PlayerBG.ppp
PicturePreProcessor.exe "resources\Gameplay Instructions overlay.png" 1 0 1 PlayerHelp.ppp
PicturePreProcessor.exe "resources\animations\login page\login%%d.png" 28 1 15 LoginMenuBG.ppp
popd

echo Building game...

@echo on
cl %CFlags% %IncludePaths% %TranslationUnits% raylibdll.lib %LFlags% %LibraryPaths%
@echo off


