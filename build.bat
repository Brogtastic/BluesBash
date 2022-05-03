@echo off
rem alway start with the correct working directory
cd %~dp0

call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"

rem ensure output directories do exist.
if not exist bin mkdir bin
if not exist bin\obj mkdir bin\obj
if not exist libs (
	echo The .\libs\ folder is missing. Please make it
	exit
)
if not exist libs\raylib (
	echo The .\libs\raylib folder is missing. Please grab a release of raylib and put it inside of the folder.
	exit
)
if not exist libs\Odin (
	echo The .\libs\Odin folder is missing. Please grab a release of Odin and put it inside of the folder.
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
%OdinC% build source\Tools\PicturePreProcessor.odin -out:bin\PicturePreProcessor.exe -debug
%OdinC% build source\Tools\UIMaker.odin -out:bin\UIMaker.exe -debug
@echo off

echo Cleaning bin\resources\processed...
if not exist bin\resources\processed mkdir bin\resources\processed
del /Q bin\resources\processed\* 

echo Running Tools...

pushd bin
PicturePreProcessor.exe resources\animations\intro\Intro%%04d.png 266 1 24 Intro.ppp
PicturePreProcessor.exe resources\animations\play\play%%d.png 5 1 24 PlayButton.ppp
PicturePreProcessor.exe resources\animations\login\login%%d.png 5 1 24 SettingsButton.ppp
PicturePreProcessor.exe resources\animations\listen\listen%%d.png 5 1 24 ListenButton.ppp
PicturePreProcessor.exe resources\animations\light\Light%%d.png 13 1 30 TopMenuLight.ppp
PicturePreProcessor.exe "resources\gameplay screen.png" 1 0 1 PlayerBG.ppp
PicturePreProcessor.exe "resources\Gameplay Instructions overlay.png" 1 0 1 PlayerHelp.ppp
PicturePreProcessor.exe "resources\animations\login page\login%%d.png" 28 1 30 LoginMenuBG.ppp
PicturePreProcessor.exe "resources\animations\curtain\curtain%%d.png" 16 1 15 curtain.ppp
PicturePreProcessor.exe "resources\animations\drumbot\drumbot%%d.png" 4 1 15 drumbot.ppp
PicturePreProcessor.exe "resources\animations\BackArrow\BackArrow%%d.png" 5 1 15 BackArrow.ppp

PicturePreProcessor.exe "resources\animations\Change Guys\Guitar\In\GuitarIn%%d.png" 3 1 15 GuitarIn.ppp
PicturePreProcessor.exe "resources\animations\Change Guys\Guitar\Out\GuitarOut%%d.png" 3 1 15 GuitarOut.ppp
PicturePreProcessor.exe "resources\animations\Change Guys\Piano\In\PianoIn%%d.png" 3 1 15 PianoIn.ppp
PicturePreProcessor.exe "resources\animations\Change Guys\Piano\Out\PianoOut%%d.png" 3 1 15 PianoOut.ppp
PicturePreProcessor.exe "resources\animations\Change Guys\Guitar\Out\GuitarOut%%d.png" 3 1 15 GuitarOut.ppp
PicturePreProcessor.exe "resources\animations\Change Guys\Sax\In\SaxIn%%d.png" 3 1 15 SaxIn.ppp
PicturePreProcessor.exe "resources\animations\Change Guys\Sax\Out\SaxOut%%d.png" 3 1 15 SaxOut.ppp
PicturePreProcessor.exe "resources\animations\ThreeBubbles\PlayerBubs\PlayerBubs%%d.png" 5 1 15 PlayerBubs.ppp
PicturePreProcessor.exe "resources\animations\ThreeBubbles\RoboBubs\RoboBubs%%d.png" 5 1 15 RoboBubs.ppp
PicturePreProcessor.exe "resources\animations\Indiv Bubbles\guitarbub\guitarbub%%d.png" 2 1 15 guiitarbub.ppp
PicturePreProcessor.exe "resources\animations\Indiv Bubbles\pianobub\pianobub%%d.png" 2 1 15 pianobub.ppp
PicturePreProcessor.exe "resources\animations\Indiv Bubbles\saxbub\saxbub%%d.png" 2 1 15 saxbub.ppp
PicturePreProcessor.exe "resources\animations\Indiv Bubbles\trombub\trombub%%d.png" 2 1 15 trombub.ppp

PicturePreProcessor.exe "resources\animations\Player Animation\Guitar\Left\GuitarLeft%%d.png" 9 1 15 GuitarLeft.ppp
PicturePreProcessor.exe "resources\animations\Player Animation\Guitar\Middle\GuitarMiddle%%d.png" 9 1 15 GuitarMiddle.ppp
PicturePreProcessor.exe "resources\animations\Player Animation\Guitar\Right\GuitarRight%%d.png" 9 1 15 GuitarRight.ppp
PicturePreProcessor.exe "resources\animations\Player Animation\Piano\Left\PianoLeft%%d.png" 9 1 15 PianoLeft.ppp
PicturePreProcessor.exe "resources\animations\Player Animation\Piano\Middle\PianoMiddle%%d.png" 9 1 15 PianoMiddle.ppp
PicturePreProcessor.exe "resources\animations\Player Animation\Piano\Right\PianoRight%%d.png" 9 1 15 PianoRight.ppp
PicturePreProcessor.exe "resources\animations\Player Animation\Sax\Left\SaxophoneLeft%%d.png" 9 1 15 SaxLeft.ppp
PicturePreProcessor.exe "resources\animations\Player Animation\Sax\Middle\SaxophoneMiddle%%d.png" 9 1 15 SaxMiddle.ppp
PicturePreProcessor.exe "resources\animations\Player Animation\Sax\Right\SaxophoneRight%%d.png" 9 1 15 SaxRight.ppp

PicturePreProcessor.exe "resources\titlescreen.png" 1 0 1 TopMenuBG.ppp
PicturePreProcessor.exe "resources\sign up.png" 1 0 1 SignUpButton.ppp
PicturePreProcessor.exe "resources\submit.png" 1 0 1 SubmitButton.ppp
PicturePreProcessor.exe "resources\textbox.png" 1 0 1 Textbox.ppp
PicturePreProcessor.exe "resources\LoginScreenShading.png" 1 0 1 LoginScreenShading.ppp
PicturePreProcessor.exe resources\animations\signuppage\SignUpScreen%%04d.png 69 1 12 SignUpScreen.ppp
popd

echo Building game...

@echo on
cl %CFlags% %IncludePaths% %TranslationUnits% raylibdll.lib %LFlags% %LibraryPaths%
@echo off


