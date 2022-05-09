#include <raylib.h>
#include <cstdio>
#include <string> 

#define Assert(Cnd) if (!(Cnd)) { __debugbreak(); }
#define ArrayCount(Array) (sizeof(Array)/sizeof(Array[0]))
//@TODO(Roskuski): Make macro for memory allocation.

// Used for static locals.
#define local_persist static

// Used for static globals/functions
#define translation_scope static
#define global_var static

// Tells MSVC what libraries we need.
#pragma comment(lib, "raylibdll.lib")
// raylib depends on these libraries.
#pragma comment(lib, "Gdi32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "Shell32.lib")

#include "BluesBash_Note.h"
#include "BluesBash_Animation.h"
#include "BluesBash_UI.h"
#include "BluesBash_Map.h"
#include "win32_BluesBash.h"
#include "Server/Commands.h" 

enum prog_state {
	GameplayScreen,
	TopMenu,
	LoginPage,
	SignUpPage,
	InstrumentSelect,
	FilterScreen,
	ListenScreen,
	PostPlayScreen,
    Intro,
};

enum sustained_key {
	Up = 0, Down, Left, Right, SustainedKey_Count,
};

struct player_info {
	int Placement;
	int LastKeyPressed;
	int LastChoice;

	double TimeUntilNextChord;
	int CurrentChord;

	note_instrument Instrument;
	note_instrument RoboInstrument;
	note_name Keyboard[19];
	note_name SustainedNotes[4];

	double NoteEaseRatio[19];

	int UserId;
	char Nickname[40];
};

#define NOT_LOGGED_IN (-1)

// Constants and globals should be defined here.
const int ScreenWidth = 1280;
const int ScreenHeight = 720;

const double BeatsPerMin = 120;
const double SecondsPerBeat = 1.f / (BeatsPerMin / 60.f);

global_var prog_state ProgState;
global_var player_info PlayerInfo;

// 4/4 time signature is assumed for these macros
#define SIXTEENTH_NOTE(Time) (Time/4)
#define EIGHTH_NOTE(Time) (Time/2)
#define QUARTER_NOTE(Time) (Time)
#define HALF_NOTE(Time) (Time*2)
#define WHOLE_NOTE(Time) (Time*4)

#include "BluesBash_Note.cpp"
#include "BluesBash_Animation.cpp"
#include "BluesBash_UI.cpp"
#include "BluesBash_Map.cpp"

void WalkToNextPlacement(int Delta, int LowBound, int HighBound, bool DoAdjust) {
	PlayerInfo.Placement += Delta;
	if (PlayerInfo.Placement < LowBound) { PlayerInfo.Placement = LowBound; }
	if (PlayerInfo.Placement > HighBound) { PlayerInfo.Placement = HighBound; }

	int Unit = 1; // @TODO(Roskuski): in the case where Delta == 0, we should try to choose a direction based on some other metric, instead of defaulting to a particular direction
	if (Delta < 0) { Unit = -1; }
	else if (Delta > 0) { Unit = 1; }
		
	while (DoAdjust) {
		if (IsNotePlaying(PlayerInfo.Keyboard[PlayerInfo.Placement],PlayerInfo.Instrument)) {
			PlayerInfo.Placement += Unit;
			if (PlayerInfo.Placement < LowBound) { PlayerInfo.Placement = LowBound; }
			if (PlayerInfo.Placement > HighBound) { PlayerInfo.Placement = HighBound; }
		}
		else if ((PlayerInfo.Placement != HighBound) && (IsNotePlaying(PlayerInfo.Keyboard[PlayerInfo.Placement+1],PlayerInfo.Instrument))) {
			PlayerInfo.Placement += Unit;
			if (PlayerInfo.Placement < LowBound) { PlayerInfo.Placement = LowBound; }
			if (PlayerInfo.Placement > HighBound) { PlayerInfo.Placement = HighBound; }
		}
		else if ((PlayerInfo.Placement != LowBound) && (IsNotePlaying(PlayerInfo.Keyboard[PlayerInfo.Placement-1],PlayerInfo.Instrument))) {
			PlayerInfo.Placement += Unit;
			if (PlayerInfo.Placement < LowBound) { PlayerInfo.Placement = LowBound; }
			if (PlayerInfo.Placement > HighBound) { PlayerInfo.Placement = HighBound; }
		}
		else { break; }

		if ((Unit == -1) && (PlayerInfo.Placement == LowBound)) { break; }
		if ((Unit == 1) && (PlayerInfo.Placement == HighBound)) { break; }
	}
}

// @TODO(Roskuski): Having other functions for non-linear interpolation would be nice!
float LinearInterp(float Start, float End, float Ratio) {
	return Start + (End - Start) * Ratio;
}

double LinearInterp(double Start, double End, double Ratio) {
	return Start + (End - Start) * Ratio;
}

void ProcessAndRenderGameplayScreen(double DeltaTime, double CurrentTime) {
	// @TODO(Roskuski): The constants here should be folded into PlayerInfo.
	const double ChordLength = WHOLE_NOTE(SecondsPerBeat);
	const chord_names ChordSequence[] = {Cmaj7, Fmaj7, Cmaj7, Gmaj7, Fmaj7, Cmaj7};
	const double ChordRatio[] = {2, 2, 2, 1, 1, 2};
    
	ui_result UIResult = {false, false};
	
	// @TODO(Roskuski): @RemoveMe move to a different initilatizion system for state init.
	local_persist bool IsInitilized = false;
	if (!IsInitilized) {
		IsInitilized = true;
		PlayerInfo.Placement = 0;
		PlayerInfo.LastChoice = KEY_LEFT;
		PlayerInfo.LastKeyPressed = KEY_LEFT;

		PlayerInfo.TimeUntilNextChord = 0;
		PlayerInfo.CurrentChord = ArrayCount(ChordSequence) - 1;

		for (int Index = 0; Index < ArrayCount(PlayerInfo.Keyboard); Index++) {
			const note_name KeyboardRef[19] = {C2, Eb2, F2, Fs2, G2, Bb2, C3, Eb3, F3, Fs3, G3, Bb3, C4, Eb4, F4, Fs4, G4, Bb4, C5};
			PlayerInfo.Keyboard[Index] = KeyboardRef[Index];
		}
		
		for (int Index = 0; Index < ArrayCount(PlayerInfo.SustainedNotes); Index++) {
			PlayerInfo.SustainedNotes[Index] = NoteName_Count;
		}
	}

	local_persist float OffsetYEaseRatio = 1;

	// @TODO(Roskuski): Right now, all of the player's key presses have the same volume.
	// We could make it so that each concurrently playing note make the next played not quiter?
	const float SustainedVolume = 0.30;
	
	if (IsKeyPressed(KEY_RIGHT)) {
		bool DoAdjust = true;
		if (PlayerInfo.LastKeyPressed == KEY_UP) { DoAdjust = false; }
			
		WalkToNextPlacement(1, 0, ArrayCount(PlayerInfo.Keyboard)-1, DoAdjust);
		PlayNoteSustained(PlayerInfo.Keyboard[PlayerInfo.Placement],PlayerInfo.Instrument, SustainedVolume);
		PlayerInfo.SustainedNotes[sustained_key::Right] = PlayerInfo.Keyboard[PlayerInfo.Placement];
		PlayerInfo.LastKeyPressed = KEY_RIGHT;
		
		OffsetYEaseRatio = 0;
	}
        
	if (IsKeyPressed(KEY_LEFT)) {
		bool DoAdjust = true;
		if (PlayerInfo.LastKeyPressed == KEY_UP) { DoAdjust = false; }
			
		WalkToNextPlacement(-1, 0, ArrayCount(PlayerInfo.Keyboard)-1, DoAdjust);
		PlayNoteSustained(PlayerInfo.Keyboard[PlayerInfo.Placement],PlayerInfo.Instrument, SustainedVolume);
		PlayerInfo.SustainedNotes[sustained_key::Left] = PlayerInfo.Keyboard[PlayerInfo.Placement];
		PlayerInfo.LastKeyPressed = KEY_LEFT;

		OffsetYEaseRatio = 0;
	}

	if (IsKeyPressed(KEY_DOWN)){
		WalkToNextPlacement(0, 0, ArrayCount(PlayerInfo.Keyboard)-1, true);
		PlayNoteSustained(PlayerInfo.Keyboard[PlayerInfo.Placement],PlayerInfo.Instrument, SustainedVolume);
		PlayerInfo.SustainedNotes[sustained_key::Down] = PlayerInfo.Keyboard[PlayerInfo.Placement];
		// @TODO(Roskuski): should we keep track of this key in PlayerInfo.LastKeyPressed?

		OffsetYEaseRatio = 0;
	}
		
	if(IsKeyPressed(KEY_UP)){
		if (PlayerInfo.LastKeyPressed == KEY_UP) {
			PlayerInfo.LastKeyPressed = PlayerInfo.LastChoice;
		}
		PlayerInfo.LastChoice = PlayerInfo.LastKeyPressed;
			
		if (PlayerInfo.LastKeyPressed == KEY_LEFT){
			WalkToNextPlacement(1, 0, ArrayCount(PlayerInfo.Keyboard)-1, false);
			PlayNoteSustained(PlayerInfo.Keyboard[PlayerInfo.Placement],PlayerInfo.Instrument, SustainedVolume);
			PlayerInfo.SustainedNotes[sustained_key::Up] = PlayerInfo.Keyboard[PlayerInfo.Placement];
		}
		else if (PlayerInfo.LastKeyPressed == KEY_RIGHT){
			WalkToNextPlacement(-1, 0, ArrayCount(PlayerInfo.Keyboard)-1, false);
			PlayNoteSustained(PlayerInfo.Keyboard[PlayerInfo.Placement],PlayerInfo.Instrument, SustainedVolume);
			PlayerInfo.SustainedNotes[sustained_key::Up] = PlayerInfo.Keyboard[PlayerInfo.Placement];
		}
		PlayerInfo.LastChoice = PlayerInfo.LastKeyPressed;
		PlayerInfo.LastKeyPressed = KEY_UP;

		OffsetYEaseRatio = 0;
	}
    
	if(IsKeyPressed(KEY_F)){
		if (PlayerInfo.Instrument == Brog_Piano) {
			for (note_name Note : Chords[ChordSequence[PlayerInfo.CurrentChord]]) {
				StopNote(Note,PlayerInfo.Instrument);
			}
			PlaySound(PianoFinale);
		}

		if (PlayerInfo.Instrument == Brog_Guitar){
			for (note_name Note : Chords[ChordSequence[PlayerInfo.CurrentChord]]) {
				StopNote(Note,PlayerInfo.Instrument);
			}
			PlaySound(GuitarFinale);
		}
		else if (PlayerInfo.Instrument == Brog_Saxophone){
			for (note_name Note : Chords[ChordSequence[PlayerInfo.CurrentChord]]) {
				StopNote(Note,PlayerInfo.Instrument);
			}
			PlaySound(SaxFinale);
		}
		ProgState = PostPlayScreen;
	}
	
	

	// Stop Sustained notes that we are no longer holding.
	for (int SustainedKey = 0; SustainedKey < SustainedKey_Count; SustainedKey++) {
		const KeyboardKey SusToRay[SustainedKey_Count] = {KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT};
		if (IsKeyReleased(SusToRay[SustainedKey])) {
			StopNoteSustained(PlayerInfo.SustainedNotes[SustainedKey],PlayerInfo.Instrument);
			PlayerInfo.SustainedNotes[SustainedKey] = NoteName_Count;
		}
	}

	// Do Chords
	//!!!!!!!!!!!!!!!!!!!!!!!!
	//-------------------------------------------------------------------------
	//WHILE F KEY NOT PRESSED
	{
		PlayerInfo.TimeUntilNextChord -= DeltaTime;
			
		if (PlayerInfo.TimeUntilNextChord <= 0) {
			// Stop the current chord (on start up we can stop notes that are not playing)
			for (note_name Note : Chords[ChordSequence[PlayerInfo.CurrentChord]]) {
				StopNote(Note,PlayerInfo.Instrument);
				PlaySound(LoopingDrumTrack);
				PlaySound(LoopingDrumTrack);
			}

			PlayerInfo.CurrentChord += 1;
			PlayerInfo.TimeUntilNextChord = ChordLength * ChordRatio[PlayerInfo.CurrentChord];
			if (PlayerInfo.CurrentChord >= ArrayCount(ChordSequence)) {
				PlayerInfo.CurrentChord = 0;
			}

			// Play the next chord
			for (note_name Note : Chords[ChordSequence[PlayerInfo.CurrentChord]]) {
				PlayNote(Note, PlayerInfo.Instrument, CurrentTime, ChordLength * ChordRatio[PlayerInfo.CurrentChord], 0, 0.25);
			}
		}
	}
		
	for (int Index = 0; Index < NoteName_Count; Index++) {
		// NOTE(Roskuski): I'm not sure if we want to move all note processing to here or not. Right now PlayingSustained plays and stops their notes elsewhere.
		switch(NoteStateList[PlayerInfo.Instrument][Index].State) {

		case QueuedForPlaying: {
			if (NoteStateList[PlayerInfo.Instrument][Index].StartTime <= CurrentTime &&
			    NoteStateList[PlayerInfo.Instrument][Index].EndTime > CurrentTime) {
				NoteStateList[PlayerInfo.Instrument][Index].State = Playing;
				SetSoundVolume(NoteSoundList[PlayerInfo.Instrument][Index], NoteStateList[PlayerInfo.Instrument][Index].Volume);
				PlaySound(NoteSoundList[PlayerInfo.Instrument][Index]);
			}
		} break;
				
		case Playing: {
			if (NoteStateList[PlayerInfo.Instrument][Index].EndTime <= CurrentTime) {
				NoteStateList[PlayerInfo.Instrument][Index].State = Stopping;
			}
		} break;

		case Stopping: {
			if (NoteStateList[PlayerInfo.Instrument][Index].FadeRatio <= 0) {
				StopSound(NoteSoundList[PlayerInfo.Instrument][Index]);
				NoteStateList[PlayerInfo.Instrument][Index].State = NotPlaying;
			}

			const double FadeTime = SIXTEENTH_NOTE(SecondsPerBeat);
			NoteStateList[PlayerInfo.Instrument][Index].FadeRatio = ((NoteStateList[PlayerInfo.Instrument][Index].FadeRatio * FadeTime) - DeltaTime)/FadeTime;

			if (NoteStateList[PlayerInfo.Instrument][Index].FadeRatio < 0) { NoteStateList[PlayerInfo.Instrument][Index].FadeRatio = 0; }
			float NewVolume = LinearInterp(0, NoteStateList[PlayerInfo.Instrument][Index].Volume, NoteStateList[PlayerInfo.Instrument][Index].FadeRatio);
				
			SetSoundVolume(NoteSoundList[PlayerInfo.Instrument][Index], NewVolume);
			if (Index == C2) {
				char Buffer[50];
				sprintf(Buffer, "Ratio: %lf, NewVolume: %lf", NoteStateList[PlayerInfo.Instrument][Index].FadeRatio, NewVolume);
				DrawText(Buffer, 10, 150, 20, BLACK);
			}
		} break;

		}
	}

	// Rendering
	{
		BeginDrawing();
		ClearBackground(RAYWHITE);

		ui_result UIResult = {false, false};
		UIResult = DoUIButtonFromMap("GameplayScreen_Background");
		UIResult = DoUIButtonFromMap("GameplayScreen_Instructions");

		UIResult = DoUIButtonFromMap("GameplayScreen_DrumBot");
		AnimateForwards(ButtonMap_Get("GameplayScreen_DrumBot"), DeltaTime, true);

		// @TODO progress bar
		DrawRectangleRec({ScreenWidth/2 - 200 - 2, ScreenHeight*(13.0/16.0) - 1, 400 + 2, 32 + 2}, BLACK);
		DrawRectangleRec({ScreenWidth/2 - 200 - 1, ScreenHeight*(13.0/16.0), 400, 32}, WHITE);
		double ChordProgress = (1 - PlayerInfo.TimeUntilNextChord/(ChordRatio[PlayerInfo.CurrentChord] * ChordLength));
		DrawRectangleRec({ScreenWidth/2 - 200 - 1, ScreenHeight*(13.0/16.0), (float)LinearInterp(0, 400, ChordProgress), 32}, GREEN);

		int KeyboardRollStartX = (ScreenWidth/2) - ((48+2) * ArrayCount(PlayerInfo.Keyboard)/2);
		int KeyboardRollAdvance = 48 + 2;
		Rectangle Rect = {(float)KeyboardRollStartX, (float)ScreenHeight - 48, 48, 48};
		for (int Index = 0; Index < ArrayCount(PlayerInfo.Keyboard); Index++) {
			note_state NoteState = NoteStateList[PlayerInfo.Instrument][PlayerInfo.Keyboard[Index]];

			Color RectColor = BLACK;
			Color TextColor = BLACK;
			switch (NoteState.State) {
			case Playing: {
				RectColor = GREEN;
			} break;
			case NotPlaying: {
				RectColor = RED;
				TextColor = WHITE;
			} break;
			case PlayingSustained: {
				RectColor = BLUE;
				TextColor = WHITE;
			} break;
			case Stopping: {
				RectColor = BLACK;
				TextColor = WHITE;
				RectColor.r = (char)(0xff * LinearInterp(1, 0, NoteState.FadeRatio));
			}
			}

			int OffsetY = 0;
			if (Index == PlayerInfo.Placement) {
				OffsetY = -48;
			}
			if (OffsetYEaseRatio >= 1) {
				OffsetYEaseRatio = 1;
			}
			else {
				OffsetYEaseRatio += DeltaTime;
			}

			OffsetY = LinearInterp(0, OffsetY, OffsetYEaseRatio);
			
			DrawRectangleRec({Rect.x-1, Rect.y-1 + OffsetY, Rect.width+2, Rect.height+2}, BLACK);
			DrawRectangleRec({Rect.x, Rect.y + OffsetY, Rect.width, Rect.height}, RectColor);
			DrawText(NoteNameStrings[PlayerInfo.Keyboard[Index]], Rect.x, Rect.y + OffsetY, 20, TextColor);
			Rect.x += KeyboardRollAdvance;
		}

		{
			local_persist animation_state Help = { "PlayerHelp", 0, 0};
			if (IsKeyDown(KEY_H)) {
				Color Trans = WHITE;
				Trans.a = 1.0 * 0xff;
				DrawTextureQuad(*GetCurrentFrame(Help), {1, 1}, {0, 0}, {0, 0, ScreenWidth, ScreenHeight}, Trans);
			}
		}
		
	//ANIMATION FOR GAMEPLAY STARTS HERE!!!------------------------------------------------------------
	if (PlayerInfo.Instrument == Brog_Piano) {
		UIResult = DoUIButtonFromMap("GameplayScreen_PlayerPiano");
		button_def *PlayerButton = ButtonMap_Get("GameplayScreen_PlayerPiano");
		if (IsKeyPressed(KEY_LEFT)) {
			free(PlayerButton->AniState.Key);
			PlayerButton->AniState.Key = (char*)malloc(strlen("PianoLeft") + 1);
			memcpy(PlayerButton->AniState.Key, "PianoLeft", strlen("PianoLeft") + 1);
			animation *NewAni = AnimationMap_Get("PianoLeft");
			PlayerButton->AniState.CurrentFrameMajor = NewAni->UniqueFrameCount - 1;
			PlayerButton->AniState.CurrentFrameMinor = NewAni->Frames[NewAni->UniqueFrameCount - 1].FrameLength;
			AnimateForwards(ButtonMap_Get("GameplayScreen_PlayerPiano"), DeltaTime, false);
		}
		if (IsKeyPressed(KEY_RIGHT)) {
			free(PlayerButton->AniState.Key);
			PlayerButton->AniState.Key = (char*)malloc(strlen("PianoRight") + 1);
			memcpy(PlayerButton->AniState.Key, "PianoRight", strlen("PianoRight") + 1);
			animation *NewAni = AnimationMap_Get("PianoRight");
			PlayerButton->AniState.CurrentFrameMajor = NewAni->Frames[NewAni->UniqueFrameCount - 1].FrameLength;
			PlayerButton->AniState.CurrentFrameMinor = NewAni->UniqueFrameCount - 1;
			AnimateForwards(ButtonMap_Get("GameplayScreen_PlayerPiano"), DeltaTime, false);
		}
		if (IsKeyPressed(KEY_UP)) {
			free(PlayerButton->AniState.Key);
			PlayerButton->AniState.Key = (char*)malloc(strlen("PianoMiddle") + 1);
			memcpy(PlayerButton->AniState.Key, "PianoMiddle", strlen("PianoMiddle") + 1);
			animation *NewAni = AnimationMap_Get("PianoMiddle");
			PlayerButton->AniState.CurrentFrameMajor = NewAni->UniqueFrameCount - 1;
			PlayerButton->AniState.CurrentFrameMinor = NewAni->Frames[NewAni->UniqueFrameCount - 1].FrameLength;
			AnimateForwards(ButtonMap_Get("GameplayScreen_PlayerPiano"), DeltaTime, false);
		}
		
	}
	
	if (PlayerInfo.Instrument == Brog_Guitar) {
		UIResult = DoUIButtonFromMap("GameplayScreen_PlayerGuitar");
		button_def *PlayerButton = ButtonMap_Get("GameplayScreen_PlayerGuitar");
		if (IsKeyPressed(KEY_LEFT)) {
			free(PlayerButton->AniState.Key);
			PlayerButton->AniState.Key = (char*)malloc(strlen("GuitarLeft") + 1);
			memcpy(PlayerButton->AniState.Key, "GuitarLeft", strlen("GuitarLeft") + 1);
			animation *NewAni = AnimationMap_Get("GuitarLeft");
			PlayerButton->AniState.CurrentFrameMajor = NewAni->UniqueFrameCount - 1;
			PlayerButton->AniState.CurrentFrameMinor = NewAni->Frames[NewAni->UniqueFrameCount - 1].FrameLength;
			AnimateForwards(ButtonMap_Get("GameplayScreen_PlayerGuitar"), DeltaTime, false);
		}
		if (IsKeyPressed(KEY_RIGHT)) {
			free(PlayerButton->AniState.Key);
			PlayerButton->AniState.Key = (char*)malloc(strlen("GuitarRight") + 1);
			memcpy(PlayerButton->AniState.Key, "GuitarRight", strlen("GuitarRight") + 1);
			animation *NewAni = AnimationMap_Get("GuitarRight");
			PlayerButton->AniState.CurrentFrameMajor = NewAni->Frames[NewAni->UniqueFrameCount - 1].FrameLength;
			PlayerButton->AniState.CurrentFrameMinor = NewAni->UniqueFrameCount - 1;
			AnimateForwards(ButtonMap_Get("GameplayScreen_PlayerGuitar"), DeltaTime, false);
		}
		if (IsKeyPressed(KEY_UP)) {
			free(PlayerButton->AniState.Key);
			PlayerButton->AniState.Key = (char*)malloc(strlen("GuitarMiddle") + 1);
			memcpy(PlayerButton->AniState.Key, "GuitarMiddle", strlen("GuitarMiddle") + 1);
			animation *NewAni = AnimationMap_Get("GuitarMiddle");
			PlayerButton->AniState.CurrentFrameMajor = NewAni->UniqueFrameCount - 1;
			PlayerButton->AniState.CurrentFrameMinor = NewAni->Frames[NewAni->UniqueFrameCount - 1].FrameLength;
			AnimateForwards(ButtonMap_Get("GameplayScreen_PlayerGuitar"), DeltaTime, false);
		}
	}
	if (PlayerInfo.Instrument == Brog_Saxophone) {
		UIResult = DoUIButtonFromMap("GameplayScreen_PlayerSax");
		button_def *PlayerButton = ButtonMap_Get("GameplayScreen_PlayerSax");
		if (IsKeyPressed(KEY_LEFT)) {
			free(PlayerButton->AniState.Key);
			PlayerButton->AniState.Key = (char*)malloc(strlen("SaxLeft") + 1);
			memcpy(PlayerButton->AniState.Key, "SaxLeft", strlen("SaxLeft") + 1);
			animation *NewAni = AnimationMap_Get("SaxLeft");
			PlayerButton->AniState.CurrentFrameMajor = NewAni->UniqueFrameCount - 1;
			PlayerButton->AniState.CurrentFrameMinor = NewAni->Frames[NewAni->UniqueFrameCount - 1].FrameLength;
			AnimateForwards(ButtonMap_Get("GameplayScreen_PlayerSax"), DeltaTime, false);
		}
		if (IsKeyPressed(KEY_RIGHT)) {
			free(PlayerButton->AniState.Key);
			PlayerButton->AniState.Key = (char*)malloc(strlen("SaxRight") + 1);
			memcpy(PlayerButton->AniState.Key, "SaxRight", strlen("SaxRight") + 1);
			animation *NewAni = AnimationMap_Get("SaxRight");
			PlayerButton->AniState.CurrentFrameMajor = NewAni->Frames[NewAni->UniqueFrameCount - 1].FrameLength;
			PlayerButton->AniState.CurrentFrameMinor = NewAni->UniqueFrameCount - 1;
			AnimateForwards(ButtonMap_Get("GameplayScreen_PlayerSax"), DeltaTime, false);
		}
		if (IsKeyPressed(KEY_UP)) {
			free(PlayerButton->AniState.Key);
			PlayerButton->AniState.Key = (char*)malloc(strlen("SaxMiddle") + 1);
			memcpy(PlayerButton->AniState.Key, "SaxMiddle", strlen("SaxMiddle") + 1);
			animation *NewAni = AnimationMap_Get("SaxMiddle");
			PlayerButton->AniState.CurrentFrameMajor = NewAni->UniqueFrameCount - 1;
			PlayerButton->AniState.CurrentFrameMinor = NewAni->Frames[NewAni->UniqueFrameCount - 1].FrameLength;
			AnimateForwards(ButtonMap_Get("GameplayScreen_PlayerSax"), DeltaTime, false);
		}
		
	}
	
	//ROBOT SHIT
	if(PlayerInfo.RoboInstrument == Brog_Piano){
		UIResult = DoUIButtonFromMap("GameplayScreen_PianoBot");
	}
	if(PlayerInfo.RoboInstrument == Brog_Guitar){
		UIResult = DoUIButtonFromMap("GameplayScreen_GuitarBot");
	}
	if(PlayerInfo.RoboInstrument == Brog_Saxophone){
		UIResult = DoUIButtonFromMap("GameplayScreen_TromboneBot");
	}
	
	button_def *BackArrow = ButtonMap_Get("GameplayScreen_BackArrow");
	UIResult = DoUIButtonFromMap("GameplayScreen_BackArrow");
	if (UIResult.PerformAction) {
		ProgState = InstrumentSelect;
	}
	if (UIResult.Hot) {
		AnimateForwards(ButtonMap_Get("GameplayScreen_BackArrow"), DeltaTime, false);
	}
	else {
		AnimateBackwards(ButtonMap_Get("GameplayScreen_BackArrow"), DeltaTime, false);
	}
	
	//ANIMATION FOR GAMEPLAY ENDS HERE!!!------------------------------------------------------------
		
		EndDrawing();
	}
}

void ProcessAndRenderTopMenu(double DeltaTime, double CurrentTime) {
	local_persist bool LightIsAnimating = false;
	local_persist double LightAnimationCooldown = 0;
	
	BeginDrawing();

	ClearBackground(RAYWHITE);

	// Do UI
	ui_result UIResult = {false, false};

	UIResult = DoUIButtonFromMap("TopMenu_Background");


	UIResult = DoUIButtonFromMap("TopMenu_Light");
	if (UIResult.Hot && LightAnimationCooldown <= 0) {
		LightIsAnimating = true;
	}
	
	if (UIResult.Hot) {
		LightAnimationCooldown = 0.5;
	}
	LightAnimationCooldown -= DeltaTime;
	
	if (LightIsAnimating) {
		button_def *Button = ButtonMap_Get("TopMenu_Light");
		if (AnimateForwards(Button->AniState, DeltaTime, false) == true) {
			LightIsAnimating = false;
			Button->AniState.CurrentFrameMajor = 0;
			Button->AniState.CurrentFrameMinor = 0;
			Button->AniState.CurrentTime = 0;
		}
	}
	
    button_def *PlayButton = ButtonMap_Get("TopMenu_Play");
	UIResult = DoUIButtonFromMap("TopMenu_Play");
	if (UIResult.PerformAction) {
		ProgState = InstrumentSelect;
	}
	if (UIResult.Hot) {
		AnimateForwards(ButtonMap_Get("TopMenu_Play"), DeltaTime, false);
	}
	else {
		AnimateBackwards(ButtonMap_Get("TopMenu_Play"), DeltaTime, false);
	}

	UIResult = DoUIButtonFromMap("TopMenu_Listen");
	if (UIResult.PerformAction) {
		ProgState = ListenScreen;
	}
	if (UIResult.Hot) {
		AnimateForwards(ButtonMap_Get("TopMenu_Listen"), DeltaTime, false);
	}
	else {
		AnimateBackwards(ButtonMap_Get("TopMenu_Listen"), DeltaTime, false);
	}	

	UIResult = DoUIButtonFromMap("TopMenu_Login");
	if (UIResult.PerformAction) {
		ProgState = LoginPage;
	}
	if (UIResult.Hot) {
		AnimateForwards(ButtonMap_Get("TopMenu_Login"), DeltaTime, false);
	}
	else {
		AnimateBackwards(ButtonMap_Get("TopMenu_Login"), DeltaTime, false);
	}

	{
		text_area_def *UserName = TextAreaMap_Get("TopMenu_DisplayUser");
		if (PlayerInfo.UserId == NOT_LOGGED_IN) {
			UserName->Buffer = "Not Logged In";
		}
		else {
			UserName->Buffer = PlayerInfo.Nickname;
		}
		DoUITextAreaFromMap("TopMenu_DisplayUser");
	}

	EndDrawing();
}

void ProcessAndRenderLoginMenu(double DeltaTime, double CurrentTime) {
	BeginDrawing();
	ClearBackground(RAYWHITE);

	local_persist char *ErrorMessage = 0;
	local_persist int ErrorMessageSize = 0;

	ui_result UIResult = {false, false};
	DoUIButtonFromMap("LoginPage_Background");
	AnimateForwards(ButtonMap_Get("LoginPage_Background"), DeltaTime, true);

	if (ErrorMessage != 0) {
		DoUIButtonFromMap("LoginPage_HoldOn");
		text_area_def *TextArea = TextAreaMap_Get("LoginPage_HoldOnMessage");
		TextArea->Buffer = ErrorMessage; // @TODO(Roskuski)This leaks memeory the first time
		TextArea->FontSize = ErrorMessageSize;
		DoUITextAreaFromMap("LoginPage_HoldOnMessage");
		UIResult = DoUIButtonFromMap("LoginPage_HoldOnOk");
		if (UIResult.PerformAction) {
			ErrorMessage = 0;
		}
	}

	UIResult = DoUITextAreaFromMap("LoginPage_EmailBox");
	if (UIResult.PerformAction) {
		DoTextInputFromMap("LoginPage_EmailBox");
	}

	UIResult = DoUITextAreaFromMap("LoginPage_PasswordBox");
	if (UIResult.PerformAction) {
		DoTextInputFromMap("LoginPage_PasswordBox");
	}
    
	button_def *BackArrow = ButtonMap_Get("LoginPage_BackArrow");
	UIResult = DoUIButtonFromMap("LoginPage_BackArrow");
	if (UIResult.PerformAction) {
		ProgState = TopMenu;
	}
	if (UIResult.Hot) {
		AnimateForwards(ButtonMap_Get("LoginPage_BackArrow"), DeltaTime, false);
	}
	else {
		AnimateBackwards(ButtonMap_Get("LoginPage_BackArrow"), DeltaTime, false);
	}

	UIResult = DoUIButtonFromMap("LoginPage_SubmitButton");
	if (UIResult.PerformAction) {
		// @TODO(Roskuski): This is placeholder.
		text_area_def *EmailBox = TextAreaMap_Get("LoginPage_EmailBox");
		text_area_def *PasswordBox = TextAreaMap_Get("LoginPage_PasswordBox");
		if (strlen(EmailBox->Buffer) == 0) {
			ErrorMessage = "Please provide an Email.";
			ErrorMessageSize = 35;
		}
		else if (strlen(PasswordBox->Buffer) == 0) {
			ErrorMessage = "Please provide a password.";
			ErrorMessageSize = 30;
		}
		else {
			net_client_nugget Send = {};
			Send.Command = Net_Client_Logon;
			memcpy(Send.Data.Logon.Email, EmailBox->Buffer, EMAIL_LEN);
			memcpy(Send.Data.Logon.Password, PasswordBox->Buffer, PASSWORD_LEN);
			net_server_nugget *Responce = ClientRequest(&Send);
			if (Responce->Command == Net_Server_LogonOk) {
				memcpy(PlayerInfo.Nickname, Responce->Data.LogonOk.Nickname, NICKNAME_LEN);
				PlayerInfo.UserId = Responce->Data.LogonOk.UserId;
				ErrorMessage = 0;
			}
			else {
				ErrorMessage = "Ensure that your Email and Password are correct.";
				ErrorMessageSize = 19;
			}
			free(Responce);
		}
	}

	UIResult = DoUIButtonFromMap("LoginPage_SignUpButton");
	if (UIResult.PerformAction) {
		ProgState = SignUpPage;
	}
	
	
	UIResult = DoUIButtonFromMap("LoginPage_LogoutButton");
	if (UIResult.PerformAction) {
		ProgState = TopMenu;
	}

	DoUIButtonFromMap("LoginPage_Shading"); 

	EndDrawing();
}

void ProcessAndRenderPostPlayScreen(double DeltaTime, double CurrentTime) {
	BeginDrawing();
	ClearBackground(RAYWHITE);

	ui_result UIResult = {false, false};
	DoUIButtonFromMap("PostPlayScreen_Background");
	AnimateForwards(ButtonMap_Get("PostPlayScreen_Background"), DeltaTime, true);

	UIResult = DoUITextAreaFromMap("PostPlayScreen_TitleTextbox");
	if (UIResult.PerformAction) {
		DoTextInputFromMap("PostPlayScreen_TitleTextbox");
	}

	UIResult = DoUIButtonFromMap("PostPlayScreen_BackToMenu");
	if (UIResult.PerformAction) {
		ProgState = TopMenu;
	}
	if (UIResult.Hot) {
		AnimateForwards(ButtonMap_Get("PostPlayScreen_BackToMenu"), DeltaTime, false);
	}
	else {
		AnimateBackwards(ButtonMap_Get("PostPlayScreen_BackToMenu"), DeltaTime, false);
	}
    
    UIResult = DoUIButtonFromMap("PostPlayScreen_UploadTrack");
	if (UIResult.PerformAction) {
		ProgState = ListenScreen;
	}
	if (UIResult.Hot) {
		AnimateForwards(ButtonMap_Get("PostPlayScreen_UploadTrack"), DeltaTime, false);
	}
	else {
		AnimateBackwards(ButtonMap_Get("PostPlayScreen_UploadTrack"), DeltaTime, false);
	}

	EndDrawing();
}

void ProcessAndRenderFilterScreen(double DeltaTime, double CurrentTime) {
	BeginDrawing();
	ClearBackground(RAYWHITE);

	ui_result UIResult = {false, false};
	DoUIButtonFromMap("FilterScreen_Background");

	UIResult = DoUITextAreaFromMap("FilterScreen_UserBox");
	if (UIResult.PerformAction) {
		DoTextInputFromMap("FilterScreen_UserBox");
	}

	button_def *BackArrow = ButtonMap_Get("FilterScreen_BackArrow");
	UIResult = DoUIButtonFromMap("FilterScreen_BackArrow");
	if (UIResult.PerformAction) {
		ProgState = ListenScreen;
	}
	if (UIResult.Hot) {
		AnimateForwards(ButtonMap_Get("FilterScreen_BackArrow"), DeltaTime, false);
	}
	else {
		AnimateBackwards(ButtonMap_Get("FilterScreen_BackArrow"), DeltaTime, false);
	}

	EndDrawing();
}

void ProcessAndRenderSignUpMenu(double DeltaTime, double CurrentTime) {
	BeginDrawing();
	ClearBackground(RAYWHITE);

	local_persist char *ErrorMessage = 0;
	local_persist int ErrorMessageSize;

	ui_result UIResult = {false, false};
	DoUIButtonFromMap("SignUpPage_Background");

	UIResult = DoUIButtonFromMap("SignUpPage_BackArrow");
	if (UIResult.PerformAction) {
		ProgState = TopMenu;
	}
	if (UIResult.Hot) {
		AnimateForwards(ButtonMap_Get("SignUpPage_BackArrow"), DeltaTime, false);
	}
	else {
		AnimateBackwards(ButtonMap_Get("SignUpPage_BackArrow"), DeltaTime, false);
	}

	if (ErrorMessage) {
		DoUIButtonFromMap("SignUpPage_HoldOnBox");
		text_area_def *TextArea = TextAreaMap_Get("SignUpPage_HoldOnText");
		TextArea->Buffer = ErrorMessage; // @TODO(Roskuski): This leaks memeory
		TextArea->FontSize = ErrorMessageSize;
		DoUITextAreaFromMap("SignUpPage_HoldOnText");

		UIResult = DoUIButtonFromMap("SignUpPage_HoldOnOk");
		if (UIResult.PerformAction) {
			ErrorMessage = 0;
		}
	}

	UIResult = DoUITextAreaFromMap("SignUpPage_Email");
	if (UIResult.PerformAction) {
		DoTextInputFromMap("SignUpPage_Email");
	}

	UIResult = DoUITextAreaFromMap("SignUpPage_Password");
	if (UIResult.PerformAction) {
		DoTextInputFromMap("SignUpPage_Password");
	}

	UIResult = DoUITextAreaFromMap("SignUpPage_Username");
	if (UIResult.PerformAction) {
		DoTextInputFromMap("SignUpPage_Username");
	}

	UIResult = DoUITextAreaFromMap("SignUpPage_SecurityQuestion");
	if (UIResult.PerformAction) {
		DoTextInputFromMap("SignUpPage_SecurityQuestion");
	}

	UIResult = DoUITextAreaFromMap("SignUpPage_SecurityAnswer");
	if (UIResult.PerformAction) {
		DoTextInputFromMap("SignUpPage_SecurityAnswer");
	}

	UIResult = DoUIButtonFromMap("SignUpPage_Submit");
	if (UIResult.PerformAction) {
		text_area_def *EmailBox = TextAreaMap_Get("SignUpPage_Email");
		text_area_def *PasswordBox = TextAreaMap_Get("SignUpPage_Password");
		text_area_def *UsernameBox = TextAreaMap_Get("SignUpPage_Username");
		text_area_def *SecurityQuestionBox = TextAreaMap_Get("SignUpPage_SecurityQuestion");
		text_area_def *SecurityAnswerBox = TextAreaMap_Get("SignUpPage_SecurityAnswer");
		net_client_nugget Send = {};
		Send.Command = Net_Client_Register;
		memcpy(Send.Data.Register.Email, EmailBox->Buffer, EMAIL_LEN);
		memcpy(Send.Data.Register.Password, PasswordBox->Buffer, PASSWORD_LEN);
		memcpy(Send.Data.Register.Nickname, UsernameBox->Buffer, NICKNAME_LEN);
		memcpy(Send.Data.Register.SecQuestion, SecurityQuestionBox->Buffer, SEC_QUESTION_LEN);
		memcpy(Send.Data.Register.SecAnswer, SecurityAnswerBox->Buffer, SEC_ANSWER_LEN);
		net_server_nugget *Responce = ClientRequest(&Send);
		// @TODO(Roskuski): This is placeholder.
		if (Responce->Command == Net_Server_RegisterOk) {
			PlayerInfo.UserId = Responce->Data.RegisterOk.UserId;
			memcpy(PlayerInfo.Nickname, Responce->Data.RegisterOk.Nickname, NICKNAME_LEN);
			ProgState = TopMenu;
		}
		else if (Responce->Command == Net_Server_RegisterFail) {
			switch (Responce->Data.RegisterFail.Reason) {
				case Net_Server_RegisterFail_Generic:
					ErrorMessage = "Generic Error Message";
					ErrorMessageSize = 40;
					break;
				case Net_Server_RegisterFail_EmptyEmail:
					ErrorMessage = "Please provide an Email";
					ErrorMessageSize = 40;
					break;
				case Net_Server_RegisterFail_EmptyPassword:
					ErrorMessage = "Please provide a Password";
					ErrorMessageSize = 36;
					break;
				case Net_Server_RegisterFail_EmptyNickname: 
					ErrorMessage = "Please provide a Username";
					ErrorMessageSize = 36;
					break;
				case Net_Server_RegisterFail_EmptySecQuestion: 
					ErrorMessage = "Please provide a security question";
					ErrorMessageSize = 28;
					break;
				case Net_Server_RegisterFail_EmptySecAnswer: 
					ErrorMessage = "Please provide a security answer";
					ErrorMessageSize = 29;
					break;
				case Net_Server_RegisterFail_UsedEmail: 
					ErrorMessage = "That email is already used.";
					ErrorMessageSize = 37;
					break;
				case Net_Server_RegisterFail_UsedNickname: 
					ErrorMessage = "That nickname is already used.";
					ErrorMessageSize = 31;
					break;
			}
		}
		free(Responce);
	}

	EndDrawing();
}

void ProcessAndRenderInstrumentSelect(double DeltaTime, double CurrentTime) {
	BeginDrawing();

	ui_result UIResult = {false, false};
	UIResult = DoUIButtonFromMap("InstrumentSelectPage_Background");
	
	UIResult = DoUIButtonFromMap("InstrumentSelectPage_Instructions");

    UIResult = DoUIButtonFromMap("InstrumentSelectPage_Player");
	UIResult = DoUIButtonFromMap("InstrumentSelectPage_BubBox");
	if (UIResult.Hot) {
        button_def *PlayerButton = ButtonMap_Get("InstrumentSelectPage_Player");
		if (AnimateForwards(ButtonMap_Get("InstrumentSelectPage_BubBox"), DeltaTime, false)) {
            button_def *PlayerButton = ButtonMap_Get("InstrumentSelectPage_Player");
            UIResult = DoUIButtonFromMap("InstrumentSelectPage_PianoBub");
            if (UIResult.PerformAction) {
                AnimateForwards(ButtonMap_Get("InstrumentSelectPage_Player"), DeltaTime, false);
                free(PlayerButton->AniState.Key);
                PlayerButton->AniState.Key = (char*)malloc(strlen("Piano") + 1);
                memcpy(PlayerButton->AniState.Key, "Piano", strlen("Piano") + 1);
                animation *NewAni = AnimationMap_Get("Piano");
                PlayerButton->AniState.CurrentFrameMajor = NewAni->UniqueFrameCount - 1;
                PlayerButton->AniState.CurrentFrameMinor = NewAni->Frames[NewAni->UniqueFrameCount - 1].FrameLength;
                PlayerInfo.Instrument = Brog_Piano;
            }
            UIResult = DoUIButtonFromMap("InstrumentSelectPage_SaxBub");
            if (UIResult.PerformAction) {
                AnimateForwards(ButtonMap_Get("InstrumentSelectPage_Player"), DeltaTime, false);
                free(PlayerButton->AniState.Key);
                PlayerButton->AniState.Key = (char*)malloc(strlen("Sax") + 1);
                memcpy(PlayerButton->AniState.Key, "Sax", strlen("Sax") + 1);
                animation *NewAni = AnimationMap_Get("Sax");
                PlayerButton->AniState.CurrentFrameMajor = NewAni->UniqueFrameCount - 1;
                PlayerButton->AniState.CurrentFrameMinor = NewAni->Frames[NewAni->UniqueFrameCount - 1].FrameLength;
                PlayerInfo.Instrument = Brog_Saxophone;
            }
            UIResult = DoUIButtonFromMap("InstrumentSelectPage_GuitarBub");
            if (UIResult.PerformAction) {
                AnimateForwards(ButtonMap_Get("InstrumentSelectPage_Player"), DeltaTime, false);
                free(PlayerButton->AniState.Key);
                PlayerButton->AniState.Key = (char*)malloc(strlen("Guitar") + 1);
                memcpy(PlayerButton->AniState.Key, "Guitar", strlen("Guitar") + 1);
                animation *NewAni = AnimationMap_Get("Guitar");
                PlayerButton->AniState.CurrentFrameMajor = NewAni->UniqueFrameCount - 1;
                PlayerButton->AniState.CurrentFrameMinor = NewAni->Frames[NewAni->UniqueFrameCount - 1].FrameLength;
                PlayerInfo.Instrument = Brog_Guitar;
            }
        }
	}
    else {
		AnimateBackwards(ButtonMap_Get("InstrumentSelectPage_BubBox"), DeltaTime, false);
	}
    
	//ROBOT BUBBLE STUFFFF
	UIResult = DoUIButtonFromMap("InstrumentSelectPage_Robot");
	UIResult = DoUIButtonFromMap("InstrumentSelectPage_RoboBubBox");
	if (UIResult.Hot) {
		button_def *RoboButton = ButtonMap_Get("InstrumentSelectPage_Robot");
		if (AnimateForwards(ButtonMap_Get("InstrumentSelectPage_RoboBubBox"), DeltaTime, false)) {
			UIResult = DoUIButtonFromMap("InstrumentSelectPage_RPianoBub");
			if (UIResult.PerformAction) {
				free(RoboButton->AniState.Key);
				RoboButton->AniState.Key = (char*)malloc(strlen("PianoBot") + 1);
				memcpy(RoboButton->AniState.Key, "PianoBot", strlen("PianoBot") + 1);
				animation *NewAni = AnimationMap_Get("PianoBot");
				RoboButton->AniState.CurrentFrameMajor = NewAni->UniqueFrameCount - 1;
				RoboButton->AniState.CurrentFrameMinor = NewAni->Frames[NewAni->UniqueFrameCount - 1].FrameLength;
				PlayerInfo.RoboInstrument = Brog_Piano;
			}
			UIResult = DoUIButtonFromMap("InstrumentSelectPage_RTromBub");
			if (UIResult.PerformAction) {
				free(RoboButton->AniState.Key);
				RoboButton->AniState.Key = (char*)malloc(strlen("TromboneBot") + 1);
				memcpy(RoboButton->AniState.Key, "TromboneBot", strlen("TromboneBot") + 1);
				animation *NewAni = AnimationMap_Get("TromboneBot");
				RoboButton->AniState.CurrentFrameMajor = NewAni->UniqueFrameCount - 1;
				RoboButton->AniState.CurrentFrameMinor = NewAni->Frames[NewAni->UniqueFrameCount - 1].FrameLength;
				PlayerInfo.RoboInstrument = Brog_Saxophone;
			}
			UIResult = DoUIButtonFromMap("InstrumentSelectPage_RGuitarBub");
			if (UIResult.PerformAction) {
				free(RoboButton->AniState.Key);
				RoboButton->AniState.Key = (char*)malloc(strlen("GuitarBot") + 1);
				memcpy(RoboButton->AniState.Key, "GuitarBot", strlen("GuitarBot") + 1);
				animation *NewAni = AnimationMap_Get("GuitarBot");
				RoboButton->AniState.CurrentFrameMajor = NewAni->UniqueFrameCount - 1;
				RoboButton->AniState.CurrentFrameMinor = NewAni->Frames[NewAni->UniqueFrameCount - 1].FrameLength;
				PlayerInfo.RoboInstrument = Brog_Guitar;
			}
		}
	}
    else {
		AnimateBackwards(ButtonMap_Get("InstrumentSelectPage_RoboBubBox"), DeltaTime, false);
	}

	button_def *GoJam = ButtonMap_Get("InstrumentSelectPage_GoJam");
	UIResult = DoUIButtonFromMap("InstrumentSelectPage_GoJam");
	if (UIResult.PerformAction) {
		ProgState = GameplayScreen;
	}
	if (UIResult.Hot) {
		AnimateForwards(ButtonMap_Get("InstrumentSelectPage_GoJam"), DeltaTime, true);
	}
	else {
		AnimateBackwards(ButtonMap_Get("InstrumentSelectPage_GoJam"), DeltaTime, false);
	}
    
    button_def *BackArrow = ButtonMap_Get("InstrumentSelectPage_BackArrow");
	UIResult = DoUIButtonFromMap("InstrumentSelectPage_BackArrow");
	if (UIResult.PerformAction) {
		ProgState = TopMenu;
	}
	if (UIResult.Hot) {
		AnimateForwards(ButtonMap_Get("InstrumentSelectPage_BackArrow"), DeltaTime, false);
	}
	else {
		AnimateBackwards(ButtonMap_Get("InstrumentSelectPage_BackArrow"), DeltaTime, false);
	}
    
    
    button_def *BotBotBot = ButtonMap_Get("InstrumentSelectPage_BotBotBot");
	UIResult = DoUIButtonFromMap("InstrumentSelectPage_BotBotBot");
	if (UIResult.PerformAction) {
		PlaySound(LoopingDrumTrack);
	}
	if(UIResult.Hot){
			AnimateForwards(ButtonMap_Get("InstrumentSelectPage_BotBotBot"), DeltaTime, true);
	}

	UIResult = DoUIButtonFromMap("InstrumentSelectPage_Player");
	AnimateBackwards(ButtonMap_Get("InstrumentSelectPage_Player"), DeltaTime, false);
	AnimateBackwards(ButtonMap_Get("InstrumentSelectPage_Robot"), DeltaTime, false);
	EndDrawing();
}


//BEGIN NEW SHIT
//---------------------------------------------------------
//BROG LOVES CODING
void ProcessAndRenderListenScreen(double DeltaTime, double CurrentTime) {
	BeginDrawing();

	ClearBackground(RAYWHITE);

	// Do UI
	ui_result UIResult = {false, false};

	UIResult = DoUIButtonFromMap("ListenScreen_Background");

	
    button_def *PlayButton = ButtonMap_Get("ListenScreen_BackArrow");
	UIResult = DoUIButtonFromMap("ListenScreen_BackArrow");
	if (UIResult.PerformAction) {
		ProgState = TopMenu;
	}
	if (UIResult.Hot) {
		AnimateForwards(ButtonMap_Get("ListenScreen_BackArrow"), DeltaTime, false);
	}
	else {
		AnimateBackwards(ButtonMap_Get("ListenScreen_BackArrow"), DeltaTime, false);
	}

	UIResult = DoUIButtonFromMap("ListenScreen_Filter");
	if (UIResult.PerformAction) {
		ProgState = FilterScreen;
	}
	if (UIResult.Hot) {
		AnimateForwards(ButtonMap_Get("ListenScreen_Filter"), DeltaTime, false);
	}
	else {
		AnimateBackwards(ButtonMap_Get("ListenScreen_Filter"), DeltaTime, false);
	}	

	EndDrawing();
}


void ProcessAndRenderIntro(double DeltaTime, double CurrentTime) {
	BeginDrawing();

	ClearBackground(RAYWHITE);

	// Do UI
	ui_result UIResult = {false, false};

	UIResult = DoUIButtonFromMap("Intro_Intro");
	
	local_persist bool PlayOnce = false;
	if (!PlayOnce && DeltaTime != 0) { // NOTE(Roskuski): Start on the first real frame
		PlaySound(BBIntroAudio);
		PlayOnce = true;
	}
	if (AnimateForwards(ButtonMap_Get("Intro_Intro"), DeltaTime, false)) {
		ProgState = TopMenu;
	}

	EndDrawing();

	//@TODO(Brog):---------------HEY ROSKUSKI MASK THE LOADING BEHIND THIS BIG NELLY OK????-----------------
}


//END NEW SHIT

int main(void) {
	// Initialization
	//--------------------------------------------------------------------------------------
	InitWindow(ScreenWidth, ScreenHeight, "Blues Bash");
	SetTargetFPS(60);

	ProgState = Intro;
	PlayerInfo.UserId = NOT_LOGGED_IN;

	AnimationMap_Init();
	ButtonMap_Init();
	TextAreaMap_Init();

	LoadAllPppFiles();
	LoadAllUimFiles();

	InitAudioDevice();

// Load all notes
	for (int InstrumentIndex = 0; InstrumentIndex < NoteInstrumentCount; InstrumentIndex++) {        
		for (int Index = 0; Index < NoteName_Count; Index++) {
			NoteSoundList[InstrumentIndex][Index] = LoadSound(NoteFileNames[InstrumentIndex][Index]);
		}
	}
	GuitarFinale = LoadSound("resources/Guitar Finale.mp3");
	PianoFinale = LoadSound("resources/Piano Finale.mp3");
	SaxFinale = LoadSound("resources/Sax Finale.mp3");
	BBIntroAudio = LoadSound("resources/BBIntroAudio.wav");
	LoopingDrumTrack = LoadSound("resources/LoopingDrumTrack.wav");

	bool InitStatus = InitNetwork();

	if (!InitStatus) { return -1; }
  
	while(!WindowShouldClose()) {
		local_persist bool IsLoadingTimeFrame = true;
		local_persist double CurrentTime = 0;

		double DeltaTime = GetFrameTime();
		if (IsLoadingTimeFrame && DeltaTime != 0) {
			DeltaTime = 0;
			IsLoadingTimeFrame = false;
		}
		CurrentTime += DeltaTime;

		switch(ProgState) {
		case GameplayScreen: {
			ProcessAndRenderGameplayScreen(DeltaTime, CurrentTime);
		} break;

		case TopMenu: {
			ProcessAndRenderTopMenu(DeltaTime, CurrentTime);
		} break;

		case LoginPage: {
			ProcessAndRenderLoginMenu(DeltaTime, CurrentTime);
		} break;

		case SignUpPage: {
			ProcessAndRenderSignUpMenu(DeltaTime, CurrentTime);
		} break;

		case InstrumentSelect: {
			ProcessAndRenderInstrumentSelect(DeltaTime, CurrentTime);
		} break;
        
		case ListenScreen: {
			ProcessAndRenderListenScreen(DeltaTime, CurrentTime);
		} break;
        
		case FilterScreen: {
			ProcessAndRenderFilterScreen(DeltaTime, CurrentTime);
		} break;
        
		case PostPlayScreen: {
			ProcessAndRenderPostPlayScreen(DeltaTime, CurrentTime);
		} break;
        
		case Intro: {
			ProcessAndRenderIntro(DeltaTime, CurrentTime);
		} break;

		default: Assert(false); break;
		}
	}

	// De-Initialization
	//--------------------------------------------------------------------------------------
	CloseAudioDevice();         // Close audio device (music streaming is automatically stopped)
	for (int Index = 0; Index < NoteName_Count; Index++) {
		UnloadSound(NoteSoundList[Brog_Piano][Index]);
	}
    
	CloseWindow();              // Close window and OpenGL context
	//--------------------------------------------------------------------------------------

	return 0;
}
