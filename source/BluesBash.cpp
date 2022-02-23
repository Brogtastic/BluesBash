#include <raylib.h>
#include <cstdio>
#include <string> 

#define Assert(Cnd) if (!(Cnd)) { __debugbreak(); }
#define ArrayCount(Array) (sizeof(Array)/sizeof(Array[0]))

// Used for static locals.
#define local_persist static

// Used for static globals/functions
#define translation_scope static
#define global_var static

// Tells MSVC what libraries we need.
#pragma comment(lib, "raylib.lib")
// raylib depends on these libraries.
#pragma comment(lib, "Gdi32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "Shell32.lib")

/*
  To Do:
  Make notes fade out instead of stopping right away
  Right and Left keys pressed make it so the two notes are not right next to each other
  Trills
  Drum track
  Right now, one note cannot play more than once. We might have to move away from "Sound" to something that allows for the same note to be played twice at once. perhpas this is called "wave" in raylib?
*/

#include "BluesBash_Note.h"
#include "BluesBash_Animation.h"
#include "BluesBash_UI.h"

enum prog_state {
	Player,
	TopMenu,
};

enum sustained_key {
	Up = 0, Down, Left, Right, SustainedKey_Count,
};

struct player_info {
	int Placement;
	int LastKeyPressed;
	int LastChoice;
	note_name Keyboard[19];
	note_name SustainedNotes[4];
};

// Constants and globals should be defined here.
const int ScreenWidth = 1280;
const int ScreenHeight = 720;

const float BeatsPerMin = 120;
const float SecondsPerBeat = 1.f / (BeatsPerMin / 60.f);

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

void WalkToNextPlacement(int Delta, int LowBound, int HighBound, bool DoAdjust) {
	PlayerInfo.Placement += Delta;
	if (PlayerInfo.Placement < LowBound) { PlayerInfo.Placement = LowBound; }
	if (PlayerInfo.Placement > HighBound) { PlayerInfo.Placement = HighBound; }

	int Unit = 1; // @TODO(Roskuski): in the case where Delta == 0, we should try to choose a direction based on some other metric, instead of defaulting to a particular direction
	if (Delta < 0) { Unit = -1; }
	else if (Delta > 0) { Unit = 1; }
		
	while (DoAdjust) {
		if (IsNotePlaying(PlayerInfo.Keyboard[PlayerInfo.Placement])) {
			PlayerInfo.Placement += Unit;
			if (PlayerInfo.Placement < LowBound) { PlayerInfo.Placement = LowBound; }
			if (PlayerInfo.Placement > HighBound) { PlayerInfo.Placement = HighBound; }
		}
		else if ((PlayerInfo.Placement != HighBound) && (IsNotePlaying(PlayerInfo.Keyboard[PlayerInfo.Placement+1]))) {
			PlayerInfo.Placement += Unit;
			if (PlayerInfo.Placement < LowBound) { PlayerInfo.Placement = LowBound; }
			if (PlayerInfo.Placement > HighBound) { PlayerInfo.Placement = HighBound; }
		}
		else if ((PlayerInfo.Placement != LowBound) && (IsNotePlaying(PlayerInfo.Keyboard[PlayerInfo.Placement-1]))) {
			PlayerInfo.Placement += Unit;
			if (PlayerInfo.Placement < LowBound) { PlayerInfo.Placement = LowBound; }
			if (PlayerInfo.Placement > HighBound) { PlayerInfo.Placement = HighBound; }
		}
		else { break; }

		if ((Unit == -1) && (PlayerInfo.Placement == LowBound)) { break; }
		if ((Unit == 1) && (PlayerInfo.Placement == HighBound)) { break; }
	}
}

float Lerp(float Start, float End, float Ratio) {
	return Start + (End - Start) * Ratio;
}

void ProcessAndRenderPlayer(float DeltaTime, float CurrentTime) {
	// @TODO(Roskuski): @RemoveMe move to a different initilatizion system for state init.
	local_persist bool IsInitilized = false;
	if (!IsInitilized) {
		IsInitilized = true;
		PlayerInfo.Placement = 0;
		PlayerInfo.LastChoice = KEY_LEFT;
		PlayerInfo.LastKeyPressed = KEY_LEFT;

		for (int Index = 0; Index < ArrayCount(PlayerInfo.Keyboard); Index++) {
			const note_name KeyboardRef[19] = {C2, Eb2, F2, Fs2, G2, Bb2, C3, Eb3, F3, Fs3, G3, Bb3, C4, Eb4, F4, Fs4, G4, Bb4, C5};
			PlayerInfo.Keyboard[Index] = KeyboardRef[Index];
		}
		
		for (int Index = 0; Index < ArrayCount(PlayerInfo.SustainedNotes); Index++) {
			PlayerInfo.SustainedNotes[Index] = NoteName_Count;
		}
	}

	// @TODO(Roskuski): Right now, all of the player's key presses have the same volume.
	// We could make it so that each concurrently playing note make the next played not quiter?
	const float SustainedVolume = 0.30;
	
	if (IsKeyPressed(KEY_RIGHT)) {
		bool DoAdjust = true;
		if (PlayerInfo.LastKeyPressed == KEY_UP) { DoAdjust = false; }
			
		WalkToNextPlacement(1, 0, ArrayCount(PlayerInfo.Keyboard)-1, DoAdjust);
		PlayNoteSustained(PlayerInfo.Keyboard[PlayerInfo.Placement], SustainedVolume);
		PlayerInfo.SustainedNotes[sustained_key::Right] = PlayerInfo.Keyboard[PlayerInfo.Placement];
		PlayerInfo.LastKeyPressed = KEY_RIGHT;
	}
        
	if (IsKeyPressed(KEY_LEFT)) {
		bool DoAdjust = true;
		if (PlayerInfo.LastKeyPressed == KEY_UP) { DoAdjust = false; }
			
		WalkToNextPlacement(-1, 0, ArrayCount(PlayerInfo.Keyboard)-1, DoAdjust);
		PlayNoteSustained(PlayerInfo.Keyboard[PlayerInfo.Placement], SustainedVolume);
		PlayerInfo.SustainedNotes[sustained_key::Left] = PlayerInfo.Keyboard[PlayerInfo.Placement];
		PlayerInfo.LastKeyPressed = KEY_LEFT;
	}

	if (IsKeyPressed(KEY_DOWN)){
		WalkToNextPlacement(0, 0, ArrayCount(PlayerInfo.Keyboard)-1, true);
		PlayNoteSustained(PlayerInfo.Keyboard[PlayerInfo.Placement], SustainedVolume);
		PlayerInfo.SustainedNotes[sustained_key::Down] = PlayerInfo.Keyboard[PlayerInfo.Placement];
		// @TODO(Roskuski): should we keep track of this key in PlayerInfo.LastKeyPressed?
	}
		
	if(IsKeyPressed(KEY_UP)){
		if (PlayerInfo.LastKeyPressed == KEY_UP) {
			PlayerInfo.LastKeyPressed = PlayerInfo.LastChoice;
		}
		PlayerInfo.LastChoice = PlayerInfo.LastKeyPressed;
			
		if (PlayerInfo.LastKeyPressed == KEY_LEFT){
			WalkToNextPlacement(1, 0, ArrayCount(PlayerInfo.Keyboard)-1, false);
			PlayNoteSustained(PlayerInfo.Keyboard[PlayerInfo.Placement], SustainedVolume);
			PlayerInfo.SustainedNotes[sustained_key::Up] = PlayerInfo.Keyboard[PlayerInfo.Placement];
		}
		else if (PlayerInfo.LastKeyPressed == KEY_RIGHT){
			WalkToNextPlacement(-1, 0, ArrayCount(PlayerInfo.Keyboard)-1, false);
			PlayNoteSustained(PlayerInfo.Keyboard[PlayerInfo.Placement], SustainedVolume);
			PlayerInfo.SustainedNotes[sustained_key::Up] = PlayerInfo.Keyboard[PlayerInfo.Placement];
		}
		PlayerInfo.LastChoice = PlayerInfo.LastKeyPressed;
		PlayerInfo.LastKeyPressed = KEY_UP;
	}

	// Stop Sustained notes that we are no longer holding.
	for (int SustainedKey = 0; SustainedKey < SustainedKey_Count; SustainedKey++) {
		const KeyboardKey SusToRay[SustainedKey_Count] = {KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT};
		if (IsKeyReleased(SusToRay[SustainedKey])) {
			StopNoteSustained(PlayerInfo.SustainedNotes[SustainedKey]);
			PlayerInfo.SustainedNotes[SustainedKey] = NoteName_Count;
		}
	}

	// Do Chords
	{
		const float ChordLength = WHOLE_NOTE(SecondsPerBeat);
		local_persist float TimeUntilNextChord = 0;
		TimeUntilNextChord -= DeltaTime;
			
		if (TimeUntilNextChord <= 0) {
			const chord_names ChordSequence[] = {Cmaj7, Fmaj7, Cmaj7, Gmaj7, Fmaj7, Cmaj7};
			const float ChordRatio[] = {2, 2, 2, 1, 1, 2};
			local_persist int CurrentChord = ArrayCount(ChordSequence) - 1;

			// Stop the current chord (on start up we can stop notes that are not playing)
			for (note_name Note : Chords[ChordSequence[CurrentChord]]) {
				StopNote(Note, CurrentTime);
			}

			CurrentChord += 1;
			TimeUntilNextChord = ChordLength * ChordRatio[CurrentChord];
			if (CurrentChord >= ArrayCount(ChordSequence)) {
				CurrentChord = 0;
			}

			// Play the next chord
			for (note_name Note : Chords[ChordSequence[CurrentChord]]) {
				PlayNote(Note, CurrentTime, ChordLength * ChordRatio[CurrentChord], 0, 0.25);
			}
		}
	}
		
	for (int Index = 0; Index < NoteName_Count; Index++) {
		// NOTE(Roskuski): I'm not sure if we want to move all note processing to here or not. Right now PlayingSustained plays and stops their notes elsewhere.
		switch(NoteStateList[Index].State) {

		case QueuedForPlaying: {
			if (NoteStateList[Index].StartTime <= CurrentTime &&
			    NoteStateList[Index].EndTime > CurrentTime) {
				NoteStateList[Index].State = Playing;
				SetSoundVolume(NoteSoundList[Index], NoteStateList[Index].Volume);
				PlaySound(NoteSoundList[Index]);
			}
		} break;
				
		case Playing: {
			if (NoteStateList[Index].EndTime <= CurrentTime) {
				NoteStateList[Index].State = Stopping;
			}
		} break;

		case Stopping: {
			if (NoteStateList[Index].FadeRatio <= 0) {
				StopSound(NoteSoundList[Index]);
				NoteStateList[Index].State = NotPlaying;
			}

			const float FadeTime = SIXTEENTH_NOTE(SecondsPerBeat);
			//const float FadeTime = WHOLE_NOTE(SecondsPerBeat);
			NoteStateList[Index].FadeRatio = ((NoteStateList[Index].FadeRatio * FadeTime) - DeltaTime)/FadeTime;

			if (NoteStateList[Index].FadeRatio < 0) { NoteStateList[Index].FadeRatio = 0; }
			float NewVolume = Lerp(0, NoteStateList[Index].Volume, NoteStateList[Index].FadeRatio);
				
			SetSoundVolume(NoteSoundList[Index], NewVolume);
			if (Index == C2) {
				char Buffer[50];
				sprintf(Buffer, "Ratio: %lf, NewVolume: %lf", NoteStateList[Index].FadeRatio, NewVolume);
				DrawText(Buffer, 10, 150, 20, BLACK);
			}
		} break;

		}
	}

	// Rendering
	{
		BeginDrawing();
		ClearBackground(RAYWHITE);
            
		Rectangle Rect = {0, 10, 48, 48};
		for (int Index = 0; Index < NoteName_Count; Index++) {
			note_state NoteState = NoteStateList[Index];

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
				RectColor.r = (char)(0xff * Lerp(1, 0, NoteState.FadeRatio));
			}
			}
			if (Index == C2) {
				Rect.y = 60;
				Rect.x = 0;
			}
				
			DrawRectangleRec({Rect.x-1, Rect.y-1, Rect.width+2, Rect.height+2}, BLACK);
			DrawRectangleRec(Rect, RectColor);
			DrawText(NoteNameStrings[Index], Rect.x, Rect.y, 20, TextColor);
			Rect.x += 48 + 2;
		}
            
		EndDrawing();
	}
}

// @TODO(Roskuski): Find a better way to talk about titleScreen here. I want to fold it into the same system we have for animations
void ProcessAndRenderTopMenu(float DeltaTime, float CurrentTime, Texture2D titleScreen) {
	// NOTE(Roskuski): This var is used in DoUIButtonAuto
	void * const CurrentFunction = ProcessAndRenderTopMenu;

	local_persist animation_state PlayButtonState = {PlayButton, 0, 0};
	local_persist animation_state ListenButtonState = {ListenButton, 0, 0};
	local_persist animation_state SettingsButtonState = {SettingsButton, 0, 0};
	
	local_persist animation_state TopMenuLightState = {TopMenuLight, 0, 0};
	local_persist bool LightIsAnimating = false;
	local_persist float LightAnimationCooldown = 0;
	
	BeginDrawing();

	ClearBackground(RAYWHITE);
	DrawTexture(titleScreen, ScreenWidth/2 - titleScreen.width/2, ScreenHeight/2 - titleScreen.height/2, WHITE);
	
	// Do UI
	ui_result UIResult = {false, false};

	UIResult = DoUIButtonAutoId({141, 0, 561, ScreenHeight}, {150, 55, 112, 66}, TopMenuLightState);
	if (UIResult.Hot && LightAnimationCooldown <= 0) {
		LightIsAnimating = true;
	}
	
	if (UIResult.Hot) {
		LightAnimationCooldown = 0.5;
	}
	LightAnimationCooldown -= DeltaTime;
	
	if (LightIsAnimating) {
		if (AnimateForwards(TopMenuLightState, DeltaTime, false) == true) {
			LightIsAnimating = false;
			TopMenuLightState.CurrentFrame = 0;
			TopMenuLightState.CurrentTime = 0;
		}
	}
	
	UIResult = DoUIButtonAutoId({211, 330 - 64, 167, 117}, {211, 330 - 64, 167, 117}, PlayButtonState);
	
	if (UIResult.PerformAction) {
		ProgState = Player;
	}
	if (UIResult.Hot) {
		AnimateForwards(PlayButtonState, DeltaTime, false);
	}
	else {
		AnimateBackwards(PlayButtonState, DeltaTime, false);
	}

	UIResult = DoUIButtonAutoId({221, 450 - 64, 243, 134}, {221, 450 - 64, 243, 134}, ListenButtonState);

	if (UIResult.PerformAction) {
		// @TODO(Roskuski): Implment State Transition
		printf("Listen Button Action\n");
	}
	if (UIResult.Hot) {
		AnimateForwards(ListenButtonState, DeltaTime, false);
	}
	else {
		AnimateBackwards(ListenButtonState, DeltaTime, false);
	}	

	UIResult = DoUIButtonAutoId({221, 510, 331, 173}, {221, 510, 331, 173}, SettingsButtonState);

	if (UIResult.PerformAction) {
		// @TODO(Roskuski): Implment State Transition
		printf("Settings Button Action\n");
	}
	if (UIResult.Hot) {
		AnimateForwards(SettingsButtonState, DeltaTime, false);
	}
	else {
		AnimateBackwards(SettingsButtonState, DeltaTime, false);
	}

	// Debug Info
	DrawFPS(10, 10);
	{
		char Buffer[256] = {};
		sprintf(Buffer, "Hot Index: %d, Active Index: %d\nMousePos: %d %d", UIContext.Hot.Index, UIContext.Active.Index, GetMouseX(), GetMouseY());
		DrawText(Buffer, 10, 30, 20, WHITE);
	} 

	EndDrawing();
}

int main(void) {
	// Initialization
	//--------------------------------------------------------------------------------------
	InitWindow(ScreenWidth, ScreenHeight, "Blues Bash");
	SetTargetFPS(60);
	ProgState = TopMenu;

	// @TODO(Roskuski): We'll likely want to have a more sophiscated way of talking about these resouces.
	//TITLE SCREEN BG
	Image title = LoadImage("resources/titlescreen.png");
	ImageResize(&title, 1280, 720);
	Texture2D titleScreen = LoadTextureFromImage(title);
	UnloadImage(title);

	title = LoadImageFromTexture(titleScreen);
	UnloadTexture(titleScreen);

	titleScreen = LoadTextureFromImage(title);
	UnloadImage(title);

	{
		float fps15 = 1.f/15.f;
		LoadAnimationFromFiles(PlayButton, fps15, 5, "resources/animations/play/play%d.png");
		LoadAnimationFromFiles(ListenButton, fps15, 5, "resources/animations/listen/listen%d.png");
		LoadAnimationFromFiles(SettingsButton, fps15, 5, "resources/animations/settings/settings%d.png");
		LoadAnimationFromFiles(TopMenuLight, fps15, 13, "resources/animations/light/Light%d.png");
	}
	                       
	InitAudioDevice();

	// LoadAllNotes
	for (int Index = 0; Index < NoteName_Count; Index++) {
		NoteSoundList[Index] = LoadSound(NoteFileNames[Index]);
	}
	  
	while(!WindowShouldClose()) {
		float CurrentTime = GetTime();
		float DeltaTime = (float)GetFrameTime();
		// Currently, there are 4 key presses that can emit sounds.
		switch(ProgState) {
		case Player: {
			ProcessAndRenderPlayer(DeltaTime, CurrentTime);
		} break;

		case TopMenu: {
			ProcessAndRenderTopMenu(DeltaTime, CurrentTime, titleScreen);
		} break;	
		}
	}

	// De-Initialization
	//--------------------------------------------------------------------------------------
	CloseAudioDevice();         // Close audio device (music streaming is automatically stopped)
	for (int Index = 0; Index < NoteName_Count; Index++) {
		UnloadSound(NoteSoundList[Index]);
	}
    
	UnloadTexture(titleScreen);       // Texture unloading
	
	CloseWindow();              // Close window and OpenGL context
	//--------------------------------------------------------------------------------------
    

	return 0;
}
