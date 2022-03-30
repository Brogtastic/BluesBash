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
	LoginMenu,
	TEMP_PrePlayMenu,
	TEMP_ViewTop,
	TEMP_ViewDetail,
	TEMP_JamSession,
	
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
	
	note_name Keyboard[19];
	note_name SustainedNotes[4];

	double NoteEaseRatio[19];
};

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

// @TODO(Roskuski): Having other functions for non-linear interpolation would be nice!
float LinearInterp(float Start, float End, float Ratio) {
	return Start + (End - Start) * Ratio;
}

double LinearInterp(double Start, double End, double Ratio) {
	return Start + (End - Start) * Ratio;
}

void ProcessAndRenderPlayer(double DeltaTime, double CurrentTime) {
	// @TODO(Roskuski): The constants here should be folded into PlayerInfo.
	const double ChordLength = WHOLE_NOTE(SecondsPerBeat);
	const chord_names ChordSequence[] = {Cmaj7, Fmaj7, Cmaj7, Gmaj7, Fmaj7, Cmaj7};
	const double ChordRatio[] = {2, 2, 2, 1, 1, 2};
	
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
		PlayNoteSustained(PlayerInfo.Keyboard[PlayerInfo.Placement], SustainedVolume);
		PlayerInfo.SustainedNotes[sustained_key::Right] = PlayerInfo.Keyboard[PlayerInfo.Placement];
		PlayerInfo.LastKeyPressed = KEY_RIGHT;
		
		OffsetYEaseRatio = 0;
	}
        
	if (IsKeyPressed(KEY_LEFT)) {
		bool DoAdjust = true;
		if (PlayerInfo.LastKeyPressed == KEY_UP) { DoAdjust = false; }
			
		WalkToNextPlacement(-1, 0, ArrayCount(PlayerInfo.Keyboard)-1, DoAdjust);
		PlayNoteSustained(PlayerInfo.Keyboard[PlayerInfo.Placement], SustainedVolume);
		PlayerInfo.SustainedNotes[sustained_key::Left] = PlayerInfo.Keyboard[PlayerInfo.Placement];
		PlayerInfo.LastKeyPressed = KEY_LEFT;

		OffsetYEaseRatio = 0;
	}

	if (IsKeyPressed(KEY_DOWN)){
		WalkToNextPlacement(0, 0, ArrayCount(PlayerInfo.Keyboard)-1, true);
		PlayNoteSustained(PlayerInfo.Keyboard[PlayerInfo.Placement], SustainedVolume);
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

		OffsetYEaseRatio = 0;
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
		PlayerInfo.TimeUntilNextChord -= DeltaTime;
			
		if (PlayerInfo.TimeUntilNextChord <= 0) {
			// Stop the current chord (on start up we can stop notes that are not playing)
			for (note_name Note : Chords[ChordSequence[PlayerInfo.CurrentChord]]) {
				StopNote(Note);
			}

			PlayerInfo.CurrentChord += 1;
			PlayerInfo.TimeUntilNextChord = ChordLength * ChordRatio[PlayerInfo.CurrentChord];
			if (PlayerInfo.CurrentChord >= ArrayCount(ChordSequence)) {
				PlayerInfo.CurrentChord = 0;
			}

			// Play the next chord
			for (note_name Note : Chords[ChordSequence[PlayerInfo.CurrentChord]]) {
				PlayNote(Note, CurrentTime, ChordLength * ChordRatio[PlayerInfo.CurrentChord], 0, 0.25);
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

			const double FadeTime = SIXTEENTH_NOTE(SecondsPerBeat);
			NoteStateList[Index].FadeRatio = ((NoteStateList[Index].FadeRatio * FadeTime) - DeltaTime)/FadeTime;

			if (NoteStateList[Index].FadeRatio < 0) { NoteStateList[Index].FadeRatio = 0; }
			float NewVolume = LinearInterp(0, NoteStateList[Index].Volume, NoteStateList[Index].FadeRatio);
				
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
		{
			local_persist animation_state Background = { "PlayerBG", 0, 0};
			DrawTextureQuad(*GetCurrentFrame(Background), {1, 1}, {0, 0}, {0, 0, ScreenWidth, ScreenHeight}, WHITE);
		}

		// @TODO progress bar
		DrawRectangleRec({ScreenWidth/2 - 200 - 2, ScreenHeight*(13.0/16.0) - 1, 400 + 2, 32 + 2}, BLACK);
		DrawRectangleRec({ScreenWidth/2 - 200 - 1, ScreenHeight*(13.0/16.0), 400, 32}, WHITE);
		double ChordProgress = (1 - PlayerInfo.TimeUntilNextChord/(ChordRatio[PlayerInfo.CurrentChord] * ChordLength));
		DrawRectangleRec({ScreenWidth/2 - 200 - 1, ScreenHeight*(13.0/16.0), (float)LinearInterp(0, 400, ChordProgress), 32}, GREEN);

		int KeyboardRollStartX = (ScreenWidth/2) - ((48+2) * ArrayCount(PlayerInfo.Keyboard)/2);
		int KeyboardRollAdvance = 48 + 2;
		Rectangle Rect = {(float)KeyboardRollStartX, (float)ScreenHeight - 48, 48, 48};
		for (int Index = 0; Index < ArrayCount(PlayerInfo.Keyboard); Index++) {
			note_state NoteState = NoteStateList[PlayerInfo.Keyboard[Index]];

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
		
		EndDrawing();
	}
}

// @TODO(Roskuski): Find a better way to talk about titleScreen here. I want to fold it into the same system we have for animations
void ProcessAndRenderTopMenu(double DeltaTime, double CurrentTime, Texture2D titleScreen) {
	// NOTE(Roskuski): This var is used in DoUIButtonAuto
	void * const CurrentFunction = ProcessAndRenderTopMenu;

	local_persist animation_state PlayButtonState = {"PlayButton", 0, 0};
	local_persist animation_state ListenButtonState = {"ListenButton", 0, 0};
	local_persist animation_state SettingsButtonState = {"SettingsButton", 0, 0};
	
	local_persist animation_state TopMenuLightState = {"TopMenuLight", 0, 0};
	local_persist bool LightIsAnimating = false;
	local_persist double LightAnimationCooldown = 0;
	
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
			TopMenuLightState.CurrentFrameMajor = 0;
			TopMenuLightState.CurrentFrameMinor = 0;
			TopMenuLightState.CurrentTime = 0;
		}
	}
	

	// @TODO(Roskuski) @Nocommit this is what the API should look like.
	//UIResult = DoUIButtonMapLookup("TopMenu_PlayButton"); 
	// ^ becomes equlivant to the below, with values fetched from button def.
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

	InitAnimationMap();

	// @TODO(Roskuski): Fold this asset into the animation system.
	//TITLE SCREEN BG
	Image title = LoadImage("resources/titlescreen.png");
	ImageResize(&title, 1280, 720);
	Texture2D titleScreen = LoadTextureFromImage(title);
	UnloadImage(title);

	title = LoadImageFromTexture(titleScreen);
	UnloadTexture(titleScreen);

	titleScreen = LoadTextureFromImage(title);
	UnloadImage(title);

	// Load all animations
	{
		LoadAnimationFromFile("resources/processed/Intro.ppp");
		LoadAnimationFromFile("resources/processed/PlayButton.ppp");
		LoadAnimationFromFile("resources/processed/SettingsButton.ppp");
		LoadAnimationFromFile("resources/processed/ListenButton.ppp");
		LoadAnimationFromFile("resources/processed/TopMenuLight.ppp");
		LoadAnimationFromFile("resources/processed/PlayerBG.ppp");
		LoadAnimationFromFile("resources/processed/PlayerHelp.ppp");
		LoadAnimationFromFile("resources/processed/LoginMenuBG.ppp");
	}

	InitAudioDevice();

	// Load all notes
	for (int Index = 0; Index < NoteName_Count; Index++) {
		NoteSoundList[Index] = LoadSound(NoteFileNames[Index]);
	}
	  
	while(!WindowShouldClose()) {
		double DeltaTime = GetFrameTime();
		local_persist double CurrentTime = 0;
		CurrentTime += DeltaTime;

		if (IsKeyPressed(KEY_ONE)) { ProgState = LoginMenu; }
		if (IsKeyPressed(KEY_TWO)) { ProgState = TEMP_PrePlayMenu; }
		if (IsKeyPressed(KEY_THREE)) { ProgState = TEMP_ViewDetail; }
		if (IsKeyPressed(KEY_FOUR)) { ProgState = TEMP_ViewTop; }

		switch(ProgState) {
		case Player: {
			ProcessAndRenderPlayer(DeltaTime, CurrentTime);
		} break;

		case TopMenu: {
			ProcessAndRenderTopMenu(DeltaTime, CurrentTime, titleScreen);
		} break;

		case LoginMenu: {
			local_persist animation_state LoginMenuState = { "LoginMenuBG", 0, 0};
			AnimateForwards(LoginMenuState, DeltaTime, true);
			BeginDrawing();
			ClearBackground(RAYWHITE);
			DrawTextureQuad(*GetCurrentFrame(LoginMenuState), {1, 1}, {0, 0}, {0, 0, ScreenWidth, ScreenHeight}, WHITE);
			EndDrawing();
		} break;

		case TEMP_PrePlayMenu: {
			local_persist animation_state PrePlayState = {"PrePlayMenuBG", 0, 0};
			BeginDrawing();
			ClearBackground(RAYWHITE);
			DrawTextureQuad(*GetCurrentFrame(PrePlayState), {1, 1}, {0, 0}, {0, 0, ScreenWidth, ScreenHeight}, WHITE);
			EndDrawing();
		} break;

		case TEMP_ViewTop: {
			local_persist animation_state Background = {"ViewTopBG", 0, 0};
			BeginDrawing();
			ClearBackground(RAYWHITE);
			DrawTextureQuad(*GetCurrentFrame(Background), {1, 1}, {0, 0}, {0, 0, ScreenWidth, ScreenHeight}, WHITE);
			EndDrawing();
		} break;

		case TEMP_ViewDetail: {
			local_persist animation_state Background = {"ViewDetailBG", 0, 0};
			BeginDrawing();
			ClearBackground(RAYWHITE);
			DrawTextureQuad(*GetCurrentFrame(Background), {1, 1}, {0, 0}, {0, 0, ScreenWidth, ScreenHeight}, WHITE);
			EndDrawing();
		} break;

		default: Assert(false); break;
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
