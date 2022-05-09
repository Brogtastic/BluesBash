@echo off
rem alway start with the correct working directory
cd %~dp0

echo Starting Time:
time /T

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
if not exist libs\sqlite (
	echo The .\libs\sqlite folder is missing. Please grab a release of sqlite and  put it inside of the folder.
	exit
)

set Game_IncludePaths=-Ilibs\raylib\include
set Game_LibraryPaths=/LIBPATH:libs\raylib\lib\
set Game_TranslationUnits=source\BluesBash.cpp source\win32_BluesBash.cpp
set Game_CFlags=-nologo -Fobin\obj\
set Game_LFlags=-link /OUT:bin\BluesBash.exe /nologo

set Server_IncludePaths=-Ilibs\sqlite\include
set Server_LibraryPaths=
set Server_TranslationUnits=source\Server\Server.cpp libs\sqlite\include\sqlite3.c
set Server_CFlags=-nologo -Fobin\obj\
set Server_LFlags=-link /OUT:bin\BluesBash_Server.exe /nologo 


if "%1"=="debug" (
	 set Game_CFlags=%Game_CFlags% -Zi -Od -MD
	 set Game_LFlags=%Game_LFlags% /DEBUG
	 set Server_CFlags=%Server_CFlags% -Zi -Od -MD
	 set Server_LFlags=%Server_LFlags% /DEBUG
) else (
	 set Game_CFlags=%Game_CFlags% -O2 -MT
	 set Game_LFlags=%Game_LFlags% 
	 set Server_CFlags=%Server_CFlags% -O2 -MT
	 set Server_LFlags=%Server_LFlags% 
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
PicturePreProcessor.exe resources\animations\intro\Intro%%04d.png 265 1 24 Intro.ppp
PicturePreProcessor.exe resources\animations\play\play%%d.png 5 1 15 PlayButton.ppp
PicturePreProcessor.exe resources\animations\login\login%%d.png 5 1 15 SettingsButton.ppp
PicturePreProcessor.exe resources\animations\listen\listen%%d.png 5 1 15 ListenButton.ppp
PicturePreProcessor.exe resources\animations\light\Light%%d.png 13 1 15 TopMenuLight.ppp
PicturePreProcessor.exe "resources\gameplay screen.png" 1 0 1 PlayerBG.ppp
PicturePreProcessor.exe "resources\Stage.png" 1 0 1 StageBG.ppp
PicturePreProcessor.exe "resources\InstrumentSelectBG.png" 1 0 1 InstrumentSelectBG.ppp
PicturePreProcessor.exe "resources\Gameplay Instructions overlay.png" 1 0 1 PlayerHelp.ppp
PicturePreProcessor.exe "resources\FilterScreenBG.png" 1 0 1 FilterScreenBG.ppp
PicturePreProcessor.exe "resources\PostPlayScreen.png" 1 0 1 PostPlayScreen.ppp
PicturePreProcessor.exe "resources\ListenScreen.png" 1 0 1 ListenScreen.ppp
PicturePreProcessor.exe "resources\nothing.png" 1 0 1 nothing.ppp


PicturePreProcessor.exe "resources\animations\filter button\filterbutton%%d.png" 5 1 15 FilterButton.ppp
PicturePreProcessor.exe "resources\animations\TrackSpots\State%%d.png" 2 1 24 TrackSpot.ppp
PicturePreProcessor.exe "resources\animations\login page\login%%d.png" 28 1 15 LoginMenuBG.ppp
PicturePreProcessor.exe "resources\animations\curtain\curtain%%d.png" 16 1 15 curtain.ppp
PicturePreProcessor.exe "resources\animations\drumbot\drumbot%%d.png" 4 1 9 drumbot.ppp
PicturePreProcessor.exe "resources\animations\BackArrow\BackArrow%%d.png" 5 1 15 BackArrow.ppp
PicturePreProcessor.exe "resources\animations\GoJam\GoJam%%d.png" 12 1 15 GoJam.ppp
PicturePreProcessor.exe "resources\animations\YKW Gameplay Robot\YKWGameplayRobot%%d.png" 11 1 15 GamePlayRobot.ppp
PicturePreProcessor.exe "resources\animations\UploadTrack\UploadTrack%%d.png" 4 1 15 UploadTrack.ppp
PicturePreProcessor.exe "resources\animations\BackToMenu\BackToMenu%%d.png" 4 1 15 BackToMenu.ppp
PicturePreProcessor.exe "resources\Logout.png" 1 0 1 logout.ppp


PicturePreProcessor.exe "resources\animations\Change Guys\Guitar\Guitar%%d.png" 3 1 15 Guitar.ppp
PicturePreProcessor.exe "resources\animations\Change Guys\Piano\Piano%%d.png" 3 1 15 Piano.ppp
PicturePreProcessor.exe "resources\animations\Change Guys\Sax\Sax%%d.png" 3 1 15 Sax.ppp
PicturePreProcessor.exe "resources\animations\RoboChange\GuitarBot\GuitarBot%%d.png" 3 1 15 GuitarBot.ppp
PicturePreProcessor.exe "resources\animations\RoboChange\PianoBot\PianoBot%%d.png" 3 1 15 PianoBot.ppp
PicturePreProcessor.exe "resources\animations\RoboChange\TromboneBot\TromboneBot%%d.png" 3 1 15 TromboneBot.ppp
PicturePreProcessor.exe "resources\animations\ThreeBubbles\PlayerBubs\PlayerBubs%%d.png" 6 1 15 PlayerBubs.ppp
PicturePreProcessor.exe "resources\animations\ThreeBubbles\RoboBubs\RoboBubs%%d.png" 6 1 15 RoboBubs.ppp
PicturePreProcessor.exe "resources\animations\Indiv Bubbles\guitarbub\guitarbub%%d.png" 3 1 15 guitarbub.ppp
PicturePreProcessor.exe "resources\animations\Indiv Bubbles\pianobub\pianobub%%d.png" 3 1 15 pianobub.ppp
PicturePreProcessor.exe "resources\animations\Indiv Bubbles\saxbub\saxbub%%d.png" 3 1 15 saxbub.ppp
PicturePreProcessor.exe "resources\animations\Indiv Bubbles\trombub\trombub%%d.png" 3 1 15 trombub.ppp

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
cl %Game_CFlags% %Game_IncludePaths% %Game_TranslationUnits% %Game_LFlags% %Game_LibraryPaths%
@echo off

echo Building Server...
@echo on
cl %Server_CFlags% %Server_IncludePaths% %Server_TranslationUnits% %Server_LFlags% %Server_LibraryPaths%
@echo off

echo Ending Time:
time /T


