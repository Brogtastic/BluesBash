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

// NOTE(Roskuski): I'm pretty sure these are in pitch order. I'm not certain though.
// NOTE(Roskuski): this is a macro that is used to define `NoteName`, and `NoteFileNames`. This will alow us to keep those two data structurs in sync easily.
// NOTE(Roskuski): If you want to read more about this Technique, it is called "X-Macros"
// NOTE(Roskuski): In essence, we create a macro that expands to a bunch of macros that are of a known (i.e X). When then define macro X to something, and that macro then expands as well.
// NOTE(Roskuski): Remeber that macros are essentially automated Copy and Paste!


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

int main(void) {
	// Initialization
	//--------------------------------------------------------------------------------------
	const int screenWidth = 1280;
	const int screenHeight = 720;
	InitWindow(1280, 720, "Blues Bash");
    
    //TITLE SCREEN BG
    Image title = LoadImage("resources/titlescreen.png");  // Load image data into CPU memory (RAM)
    ImageResize(&title, 1280, 720);
    Texture2D titleScreen = LoadTextureFromImage(title);       // Image converted to texture, GPU memory (RAM -> VRAM)
    UnloadImage(title);                                    // Unload image data from CPU memory (RAM)

    title = LoadImageFromTexture(titleScreen);                 // Load image from GPU texture (VRAM -> RAM)
    UnloadTexture(titleScreen);                                // Unload texture from GPU memory (VRAM)

    titleScreen = LoadTextureFromImage(title);                 // Recreate texture from retrieved image data (RAM -> VRAM)
    UnloadImage(title);
    
    //PLAY BUTTON
    Image play = LoadImage("resources/animations/play/play1.png");  // Load image data into CPU memory (RAM)
    ImageResize(&play, 365, 205.35);
    Texture2D playButton = LoadTextureFromImage(play);       // Image converted to texture, GPU memory (RAM -> VRAM)
    UnloadImage(play);                                    // Unload image data from CPU memory (RAM)

    play = LoadImageFromTexture(playButton);                 // Load image from GPU texture (VRAM -> RAM)
    UnloadTexture(playButton);                                // Unload texture from GPU memory (VRAM)

    playButton = LoadTextureFromImage(play);                 // Recreate texture from retrieved image data (RAM -> VRAM)
    UnloadImage(play);
    
    //LISTEN BUTTON
    Image listen = LoadImage("resources/animations/listen/listen1.png");  // Load image data into CPU memory (RAM)
    ImageResize(&listen, 365, 205.35);
    Texture2D listenButton = LoadTextureFromImage(listen);       // Image converted to texture, GPU memory (RAM -> VRAM)
    UnloadImage(listen);                                    // Unload image data from CPU memory (RAM)

    listen = LoadImageFromTexture(listenButton);                 // Load image from GPU texture (VRAM -> RAM)
    UnloadTexture(listenButton);                                // Unload texture from GPU memory (VRAM)

    listenButton = LoadTextureFromImage(listen);                 // Recreate texture from retrieved image data (RAM -> VRAM)
    UnloadImage(listen);
    
    //SETTINGS BUTTON
    Image settings = LoadImage("resources/animations/settings/settings1.png");  // Load image data into CPU memory (RAM)
    ImageResize(&settings, 365, 205.35);
    Texture2D settingsButton = LoadTextureFromImage(settings);       // Image converted to texture, GPU memory (RAM -> VRAM)
    UnloadImage(settings);                                    // Unload image data from CPU memory (RAM)

    settings = LoadImageFromTexture(settingsButton);                 // Load image from GPU texture (VRAM -> RAM)
    UnloadTexture(settingsButton);                                // Unload texture from GPU memory (VRAM)

    settingsButton = LoadTextureFromImage(settings);                 // Recreate texture from retrieved image data (RAM -> VRAM)
    UnloadImage(settings);





	InitAudioDevice();

	// LoadAllNotes
	for (int Index = 0; Index < NoteName_Count; Index++) {
		NoteSoundList[Index] = LoadSound(NoteFileNames[Index]);
	}
	
	int Placement = 0;
	int LastKeyPressed = 0;
  
	note_name Keyboard[19] = {C2, Eb2, F2, Fs2, G2, Bb2, C3, Eb3, F3, Fs3, G3, Bb3, C4, Eb4, F4, Fs4, G4, Bb4, C5};

	enum sustained_key {
		Up = 0, Down, Left, Right, SustainedKey_Count,
	};
	note_name SustainedNotes[4] = {NoteName_Count, NoteName_Count, NoteName_Count, NoteName_Count}; // Filling this array with some bogus value.
    
	while(!WindowShouldClose()) {
		float CurrentTime = GetTime();
		float DeltaTime = (float)GetFrameTime();
		// Currently, there are 4 key presses that can emit sounds.

		if (IsKeyPressed(KEY_RIGHT)) {
			bool DoAdjust = true;
			if (LastKeyPressed == KEY_UP) { DoAdjust = false; }
			
			Placement = WalkToNextPlacement(Placement, 1, 0, ArrayCount(Keyboard)-1, Keyboard, DoAdjust);
			PlayNoteSustained(Keyboard[Placement]);
			SustainedNotes[sustained_key::Right] = Keyboard[Placement];
			LastKeyPressed = KEY_RIGHT;
		}
        
		if (IsKeyPressed(KEY_LEFT)) {
			bool DoAdjust = true;
			if (LastKeyPressed == KEY_UP) { DoAdjust = false; }
			
			Placement = WalkToNextPlacement(Placement, -1, 0, ArrayCount(Keyboard)-1, Keyboard, DoAdjust);
			PlayNoteSustained(Keyboard[Placement]);
			SustainedNotes[sustained_key::Left] = Keyboard[Placement];
			LastKeyPressed = KEY_LEFT;
		}

		if (IsKeyPressed(KEY_DOWN)){
			Placement = WalkToNextPlacement(Placement, 0, 0, ArrayCount(Keyboard)-1, Keyboard);
			PlayNoteSustained(Keyboard[Placement]);
			SustainedNotes[sustained_key::Down] = Keyboard[Placement];
			// @TODO(Roskuski): should we keep track of this key in LastKeyPressed?
		}
		
		if(IsKeyPressed(KEY_UP)){
			local_persist int LastChoice = LastKeyPressed;
			if (LastKeyPressed == KEY_UP) {
				LastKeyPressed = LastChoice;
			}
			
			if (LastKeyPressed == KEY_LEFT){
				Placement = WalkToNextPlacement(Placement, 1, 0, ArrayCount(Keyboard)-1, Keyboard, false);
				PlayNoteSustained(Keyboard[Placement]);
				SustainedNotes[sustained_key::Up] = Keyboard[Placement];
			}
			else if (LastKeyPressed == KEY_RIGHT){
				Placement = WalkToNextPlacement(Placement, -1, 0, ArrayCount(Keyboard)-1, Keyboard, false);
				PlayNoteSustained(Keyboard[Placement]);
				SustainedNotes[sustained_key::Up] = Keyboard[Placement];
			}
			LastChoice = LastKeyPressed;
			LastKeyPressed = KEY_UP;
		}

		// Stop Sustained notes that we are no longer holding.
		for (int SustainedKey = 0; SustainedKey < SustainedKey_Count; SustainedKey++) {
			const KeyboardKey SusToRay[SustainedKey_Count] = {KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT};
			if (IsKeyReleased(SusToRay[SustainedKey])) {
				StopNoteSustained(SustainedNotes[SustainedKey]);
				SustainedNotes[SustainedKey] = NoteName_Count;
			}
		}

		// Do Chords
		#if 1
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
		#else
		{
			local_persist int CurrentChord = ArrayCount(ChordSequence) - 1;
			const chord_names ChordSequence[] = {Cmaj7, Fmaj7, Cmaj7, Gmaj7, Fmaj7, };
			if (IsKeyPressed(KEY_SPACE)) {
			
			
			
				if (CurrentChord >= ArrayCount(ChordSequence)) {
					CurrentChord = 0;
				}

				for (note_name Note : Chords[ChordSequence[CurrentChord]]) {
					PlayNote(Note, CurrentTime, ChordLength);
				}
			}

			if (IsKeyReleased(KEY_SPACE)) {
				for (note_name Note : Chords[ChordSequence[CurrentChord]]) {
					StopNote(Note, CurrentTime);
				}
			}
		}|
		#endif
		
		
		for (int Index = 0; Index < NoteName_Count; Index++) {
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
            DrawTexture(titleScreen, screenWidth/2 - titleScreen.width/2, screenHeight/2 - titleScreen.height/2, WHITE);
            DrawTexture(playButton, 127, 287, WHITE);
            DrawTexture(listenButton, 168, 381, WHITE);
            DrawTexture(settingsButton, 204, 479, WHITE);
            
			Rectangle Rect = {0, 10, 32, 32};
			for (note_state NoteState : NoteStateList) {
				Color RectColor = BLACK;
				switch (NoteState.State) {
				case note_state_enum::Playing: {
					RectColor = GREEN;
				} break;
				case note_state_enum::NotPlaying: {
					RectColor = RED;
				} break;
				case note_state_enum::PlayingSustained: {
					RectColor = BLUE;
				} break;
				}
				DrawRectangleRec(Rect, RectColor);
				Rect.x += 32;
			}
            
			EndDrawing();
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
