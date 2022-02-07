#include <raylib.h>
#include <cstdio>

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

// NOTE(Brog): Here are the chords
//
//             Cmaj7 -- C1, E1, G1, Bb1
//             Fmaj7 -- C1, Eb1, F1, A1
//             Gmaj7 -- D1, F1, G1, B1 
//
//             Cmaj7 2 bars
//             Fmaj7 2 bars
//             Cmaj7 2 bars
//             Gmaj7 1 bar
//             Fmaj7 1 bar
//             Cmaj7 2 bars

/*
  To Do:
  Make notes fade out instead of stopping right away
  Right and Left keys pressed make it so the two notes are not right next to each other
  Trills
  Drum track
  Right now, one note cannot play more than once. We might have to move away from "Sound" to something that allows for the same note to be played twice at once. perhpas this is called "wave" in raylib?


*/

const int ScreenWidth = 1280;
const int ScreenHeight = 720;

// NOTE(Roskuski): this is a macro that is used to define `NoteName`, and `NoteFileNames`. This will alow us to keep those two data structurs in sync easily.
// NOTE(Roskuski): If you want to read more about this Technique, it is called "X-Macros"
// NOTE(Roskuski): In essence, we create a macro that expands to a bunch of macros that are of a known (i.e X). When then define macro X to something, and that macro then expands as well.
// NOTE(Roskuski): Remeber that macros are essentially automated Copy and Paste!

#define NOTES	  \
	XX(A1, 0) \
	X(C1) \
	X(D1) \
	X(Eb1) \
	X(E1) \
	X(F1) \
	X(Fs1) \
	X(G1) \
	X(Gs1) \
	X(Bb1) \
	X(B1) \
	  \
	X(C2) \
	X(Eb2) \
	X(F2) \
	X(Fs2) \
	X(G2) \
	X(Bb2) \
	  \
	X(C3) \
	X(Eb3) \
	X(F3) \
	X(Fs3) \
	X(G3) \
	X(Bb3) \
	  \
	X(C4) \
	X(Eb4) \
	X(F4) \
	X(Fs4) \
	X(G4) \
	X(Bb4) \
	  \
	X(C5)
		
		
enum note_name {
#define XX(Name, Value) Name = Value,
#define X(Name) Name,
	NOTES
	NoteName_Count, // This entry's value is how many notes we have.
#undef XX
#undef X	
};

const char *NoteNameStrings[] = {
#define XX(Name, Value) X(Name)
#define X(Name) #Name,
	NOTES
#undef XX
#undef X
};

const char *NoteFileNames[] = {
#define XX(Name, Value) "resources/allNotes/Brog_Piano/"#Name".mp3",
#define X(Name) "resources/allNotes/Brog_Piano/"#Name".mp3",
	NOTES
#undef XX
#undef X
};
#undef NOTES

enum chord_names { Cmaj7 = 0, Fmaj7, Gmaj7,  };
const note_name Chords[3][4] = {
	{C1, E1, G1, Bb1, }, // Cmaj7
	{C1, Eb1, F1, A1, }, // Fmaj7
	{D1, F1, G1, B1, }, // Gmaj7
};

const float BeatsPerMin = 120;
const float BeatsPerSecond = BeatsPerMin / 60;
const float SecondsPerBeat = 1 / BeatsPerSecond;

// 4/4 time signature is assumed for these macros
#define SIXTEENTH_NOTE(Time) (Time/4)
#define EIGHTH_NOTE(Time) (Time/2)
#define QUARTER_NOTE(Time) (Time)
#define HALF_NOTE(Time) (Time*2)
#define WHOLE_NOTE(Time) (Time*4)

enum note_state_enum {
	NotPlaying = 0,
	QueuedForPlaying,
	Playing,
	PlayingSustained,
	Stopping,
};

struct note_state {
	note_state_enum State;
	float StartTime;
	float EndTime;
};

note_state NoteStateList[NoteName_Count];
Sound NoteSoundList[NoteName_Count];

translation_scope inline bool IsNotePlaying(note_name Note) {
	bool Result = false;
	note_state_enum State = NoteStateList[Note].State;
	
	switch(State) {
	case Playing:
	case PlayingSustained:
		Result = true;
	}

	return Result;
}

// Plays a note for the duration that a key is held down for.
translation_scope inline void PlayNoteSustained(note_name Note) {
	Assert(Note >= 0);
	Assert(Note < NoteName_Count);
	if (NoteStateList[Note].State == NotPlaying) { // @TODO(Roskuski): This check might be detremental to the sound.
		NoteStateList[Note].State = PlayingSustained;
		// @TODO(Roskuski): I think that we're going to need to know the CurrentTime here to facilitate creating replay numbers.
		PlaySound(NoteSoundList[Note]);
	}
}

// Stops a sustained note
translation_scope inline void StopNoteSustained(note_name Note) {
	Assert(Note >= 0);
	Assert(Note < NoteName_Count);
	
	// @TODO(Roskuski): I think that we're going to need to know the CurrentTime here to facilitate creating replay numbers.
	if (NoteStateList[Note].State == PlayingSustained) {
		StopSound(NoteSoundList[Note]);
		NoteStateList[Note].State = NotPlaying;
	}
	else {
		printf("[WARNING]: We tried to stop a note that wasn't in PlayingSustained state! Doing nothing\n");
	}
}

// Length is how long the note will play.
// Delay is how long we will wait until starting to play the note.
translation_scope inline void PlayNote(note_name Note, float CurrentTime, float Length, float Delay = 0.0f) {
	Assert(Note != NoteName_Count);
	NoteStateList[Note].State = QueuedForPlaying;
	NoteStateList[Note].StartTime = CurrentTime + Delay;
	NoteStateList[Note].EndTime = CurrentTime + Delay + Length;
	
}

translation_scope inline void StopNote(note_name Note, float CurrentTime) {
	Assert(Note >= 0);
	Assert(Note != NoteName_Count);
	NoteStateList[Note].EndTime = CurrentTime;
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
		PlayerInfo.LastChoice = PlayerInfo.LastKeyPressed;
		if (PlayerInfo.LastKeyPressed == KEY_UP) {
			PlayerInfo.LastKeyPressed = PlayerInfo.LastChoice;
		}
			
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

void ProcessAndRenderTopMenu(Texture2D titleScreen, Texture2D playButton, Texture2D listenButton, Texture2D settingsButton) {

	// @TODO(Roskuski): Implment changing from top menu into other states.

	// Draw 
	{
		BeginDrawing();
		
		DrawTexture(titleScreen, ScreenWidth/2 - titleScreen.width/2, ScreenHeight/2 - titleScreen.height/2, WHITE);
		DrawTexture(playButton, 127, 287, WHITE);
		DrawTexture(listenButton, 168, 381, WHITE);
		DrawTexture(settingsButton, 204, 479, WHITE);

		EndDrawing();
	}
}

int main(void) {
	// Initialization
	//--------------------------------------------------------------------------------------
	InitWindow(ScreenWidth, ScreenHeight, "Blues Bash");
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
    
	//PLAY BUTTON
	Image play = LoadImage("resources/animations/play/play1.png");
	ImageResize(&play, 365, 205.35);
	Texture2D playButton = LoadTextureFromImage(play);
	UnloadImage(play);

	play = LoadImageFromTexture(playButton);
	UnloadTexture(playButton);

	playButton = LoadTextureFromImage(play);
	UnloadImage(play);
    
	//LISTEN BUTTON
	Image listen = LoadImage("resources/animations/listen/listen1.png");
	ImageResize(&listen, 365, 205.35);
	Texture2D listenButton = LoadTextureFromImage(listen);
	UnloadImage(listen);

	listen = LoadImageFromTexture(listenButton);
	UnloadTexture(listenButton);

	listenButton = LoadTextureFromImage(listen);
	UnloadImage(listen);
    
	//SETTINGS BUTTON
	Image settings = LoadImage("resources/animations/settings/settings1.png");
	ImageResize(&settings, 365, 205.35);
	Texture2D settingsButton = LoadTextureFromImage(settings);
	UnloadImage(settings);

	settings = LoadImageFromTexture(settingsButton);
	UnloadTexture(settingsButton);

	settingsButton = LoadTextureFromImage(settings);
	UnloadImage(settings);

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
			ProcessAndRenderTopMenu(titleScreen, playButton, listenButton, settingsButton);
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
