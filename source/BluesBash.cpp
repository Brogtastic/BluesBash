#include <raylib.h>
#include <cstdio>
#include <string> 

#define Assert(Cnd) if (!(Cnd)) { __debugbreak(); }
#define ArrayCount(Array) (sizeof(Array)/sizeof(Array[0]))

// Used for static locals.
#define local_persist static

// Used for static globals/functions
#define translation_scope static

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

struct animation {
	float FrameTime;
	float CurrentTime;
	int CurrentFrame;
	int FrameCount;
	Texture2D *Frames;
};

enum animation_enum {
	PlayButton,
	ListenButton,
	SettingsButton,
	
	AnimationEnum_Count, // NOTE(Roskuski): Keep this at the end.
};

// Constants and globals should be defined here.
const int ScreenWidth = 1280;
const int ScreenHeight = 720;

const float BeatsPerMin = 120;
const float SecondsPerBeat = 1.f / (BeatsPerMin / 60.f);

// 4/4 time signature is assumed for these macros
#define SIXTEENTH_NOTE(Time) (Time/4)
#define EIGHTH_NOTE(Time) (Time/2)
#define QUARTER_NOTE(Time) (Time)
#define HALF_NOTE(Time) (Time*2)
#define WHOLE_NOTE(Time) (Time*4)

note_state NoteStateList[NoteName_Count];
Sound NoteSoundList[NoteName_Count];

animation AnimationList[AnimationEnum_Count] = {};

#include "BluesBash_Note.cpp"

Texture2D* LoadAnimationFrames(int FrameCount, const char *PathFormatString) {
	Texture2D *Result = (Texture2D*)malloc(FrameCount * sizeof(Texture2D));
	char *Buffer = (char*)malloc(sizeof(char) * 256); // @TODO(Roskuski): We should actually calculate a safe value instead of just shooting in the dark.
	
	for(int Index = 0; Index < FrameCount; Index++){
		sprintf(Buffer, PathFormatString, Index + 1);
		Image Temp = LoadImage(Buffer);
		ImageResize(&Temp, 365, 205.35); // @TODO(Roskuski): We should change these magic numbes into named constants
		Result[Index] = LoadTextureFromImage(Temp);
		UnloadImage(Temp);
	}
	free(Buffer);
	
	return Result;
}

int WalkToNextPlacement(int Placement, int Delta, int LowBound, int HighBound, note_name *NoteList, bool DoAdjust = true) {
	Placement += Delta;
	if (Placement < LowBound) { Placement = LowBound; }
	if (Placement > HighBound) { Placement = HighBound; }

	int Unit = 1; // @TODO(Roskuski): I'm not convinced that defaulting to any particular direction is a great idea when adjancent notes are playing.
	if (Delta < 0) { Unit = -1; }
	else if (Delta > 0) { Unit = 1; }
		
	while (DoAdjust) {
		if (IsNotePlaying(NoteList[Placement])) {
			Placement += Unit;
			if (Placement < LowBound) { Placement = LowBound; }
			if (Placement > HighBound) { Placement = HighBound; }
		}
		else if ((Placement != HighBound) && (IsNotePlaying(NoteList[Placement+1]))) {
			Placement += Unit;
			if (Placement < LowBound) { Placement = LowBound; }
			if (Placement > HighBound) { Placement = HighBound; }
		}
		else if ((Placement != LowBound) && (IsNotePlaying(NoteList[Placement-1]))) {
			Placement += Unit;
			if (Placement < LowBound) { Placement = LowBound; }
			if (Placement > HighBound) { Placement = HighBound; }
		}
		else { break; }

		if ((Unit == -1) && (Placement == LowBound)) { break; }
		if ((Unit == 1) && (Placement == HighBound)) { break; }
	}
	
	return Placement;
}


enum prog_state {
	Player,
	TopMenu,
};

prog_state ProgState;

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
player_info PlayerInfo;

void ProcessAndRenderPlayer(float CurrentTime, float DeltaTime) {
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
	
	if (IsKeyPressed(KEY_RIGHT)) {
		bool DoAdjust = true;
		if (PlayerInfo.LastKeyPressed == KEY_UP) { DoAdjust = false; }
			
		PlayerInfo.Placement = WalkToNextPlacement(PlayerInfo.Placement, 1, 0, ArrayCount(PlayerInfo.Keyboard)-1, PlayerInfo.Keyboard, DoAdjust);
		PlayNoteSustained(PlayerInfo.Keyboard[PlayerInfo.Placement]);
		PlayerInfo.SustainedNotes[sustained_key::Right] = PlayerInfo.Keyboard[PlayerInfo.Placement];
		PlayerInfo.LastKeyPressed = KEY_RIGHT;
	}
        
	if (IsKeyPressed(KEY_LEFT)) {
		bool DoAdjust = true;
		if (PlayerInfo.LastKeyPressed == KEY_UP) { DoAdjust = false; }
			
		PlayerInfo.Placement = WalkToNextPlacement(PlayerInfo.Placement, -1, 0, ArrayCount(PlayerInfo.Keyboard)-1, PlayerInfo.Keyboard, DoAdjust);
		PlayNoteSustained(PlayerInfo.Keyboard[PlayerInfo.Placement]);
		PlayerInfo.SustainedNotes[sustained_key::Left] = PlayerInfo.Keyboard[PlayerInfo.Placement];
		PlayerInfo.LastKeyPressed = KEY_LEFT;
	}

	if (IsKeyPressed(KEY_DOWN)){
		PlayerInfo.Placement = WalkToNextPlacement(PlayerInfo.Placement, 0, 0, ArrayCount(PlayerInfo.Keyboard)-1, PlayerInfo.Keyboard);
		PlayNoteSustained(PlayerInfo.Keyboard[PlayerInfo.Placement]);
		PlayerInfo.SustainedNotes[sustained_key::Down] = PlayerInfo.Keyboard[PlayerInfo.Placement];
		// @TODO(Roskuski): should we keep track of this key in PlayerInfo.LastKeyPressed?
	}
		
	if(IsKeyPressed(KEY_UP)){
		if (PlayerInfo.LastKeyPressed == KEY_UP) {
			PlayerInfo.LastKeyPressed = PlayerInfo.LastChoice;
		}
		PlayerInfo.LastChoice = PlayerInfo.LastKeyPressed;
			
		if (PlayerInfo.LastKeyPressed == KEY_LEFT){
			PlayerInfo.Placement = WalkToNextPlacement(PlayerInfo.Placement, 1, 0, ArrayCount(PlayerInfo.Keyboard)-1, PlayerInfo.Keyboard, false);
			PlayNoteSustained(PlayerInfo.Keyboard[PlayerInfo.Placement]);
			PlayerInfo.SustainedNotes[sustained_key::Up] = PlayerInfo.Keyboard[PlayerInfo.Placement];
		}
		else if (PlayerInfo.LastKeyPressed == KEY_RIGHT){
			PlayerInfo.Placement = WalkToNextPlacement(PlayerInfo.Placement, -1, 0, ArrayCount(PlayerInfo.Keyboard)-1, PlayerInfo.Keyboard, false);
			PlayNoteSustained(PlayerInfo.Keyboard[PlayerInfo.Placement]);
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
				PlayNote(Note, CurrentTime, ChordLength * ChordRatio[CurrentChord]);
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
				PlaySound(NoteSoundList[Index]);
			}
		} break;
				
		case Playing: {
			if (NoteStateList[Index].EndTime <= CurrentTime) {
				StopSound(NoteSoundList[Index]);
				NoteStateList[Index].State = NotPlaying;
			}
		} break;

		case Stopping: {
			StopSound(NoteSoundList[Index]);
			NoteStateList[Index].State = NotPlaying;
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

inline Texture2D* GetCurrentFrame(animation_enum Index) {
	return &AnimationList[Index].Frames[AnimationList[Index].CurrentFrame];
}

void ProcessAndRenderTopMenu(Texture2D titleScreen) {

	// @TODO(Roskuski): Implment changing from top menu into other states.

	// Draw 
	{
		BeginDrawing();
		
		DrawTexture(titleScreen, ScreenWidth/2 - titleScreen.width/2, ScreenHeight/2 - titleScreen.height/2, WHITE);
		DrawTexture(*GetCurrentFrame(PlayButton), 127, 287, WHITE);
		DrawTexture(*GetCurrentFrame(ListenButton), 168, 381, WHITE);
		DrawTexture(*GetCurrentFrame(SettingsButton), 204, 479, WHITE);
		DrawFPS(10, 10);

		EndDrawing();
	}
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

	AnimationList[PlayButton].FrameTime = 0.05;
	AnimationList[PlayButton].Frames = 0;
	AnimationList[PlayButton].FrameCount = 8;
	AnimationList[PlayButton].CurrentFrame = 0;
	AnimationList[PlayButton].Frames = LoadAnimationFrames(AnimationList[PlayButton].FrameCount, "resources/animations/play/play%d.png");
	
	AnimationList[ListenButton].FrameTime = 0.05;
	AnimationList[ListenButton].Frames = 0;
	AnimationList[ListenButton].FrameCount = 8;
	AnimationList[ListenButton].CurrentFrame = 0;
	AnimationList[ListenButton].Frames = LoadAnimationFrames(AnimationList[ListenButton].FrameCount, "resources/animations/listen/listen%d.png");
    
	AnimationList[SettingsButton].FrameTime = 0.05;
	AnimationList[SettingsButton].Frames = 0;
	AnimationList[SettingsButton].FrameCount = 8;
	AnimationList[SettingsButton].CurrentFrame = 0;
	AnimationList[SettingsButton].Frames = LoadAnimationFrames(AnimationList[SettingsButton].FrameCount, "resources/animations/settings/settings%d.png");	
  
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
			ProcessAndRenderPlayer(CurrentTime, DeltaTime);
		} break;

		case TopMenu: {      
			// @TODO(Roskuski): This can be pulled out into it's own function. "void UpdateAniamtion(float DeltaTime)"?
			for (int Index = 0; Index < AnimationEnum_Count; Index++) {
				animation * const Animation = &AnimationList[Index];
				Animation->CurrentTime += DeltaTime;
				Animation->CurrentTime = 0;
				Animation->CurrentFrame += 1;
				if (Animation->CurrentFrame == Animation->FrameCount) {
					Animation->CurrentFrame = 0;
				}
			}     
			ProcessAndRenderTopMenu(titleScreen);
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
