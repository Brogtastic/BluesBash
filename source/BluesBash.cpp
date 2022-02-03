#include <raylib.h>
#include <cstdio>

#define Assert(Cnd) if (!(Cnd)) { __debugbreak(); }

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
//             Fmaj7 -- C1, Eb1, G1, A1
//             Gmaj7 -- D1, F1, G1, B1 
//
//             Cmaj7 4 bars
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
#define XX(Name, Value) "resources/allNotes/"#Name".mp3",
#define X(Name) "resources/allNotes/"#Name".mp3",
	NOTES
#undef XX
#undef X
};
#undef NOTES

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
};

struct note_state {
	note_state_enum State;
	float StartTime;
	float EndTime;
};

note_state NoteStateList[NoteName_Count];
Sound NoteSoundList[NoteName_Count];

static inline bool IsNotePlaying(note_name Note) {
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
static inline void PlayNoteSustained(note_name Note) {
	Assert(Note >= 0);
	Assert(Note < NoteName_Count);
	if (NoteStateList[Note].State == NotPlaying) { // @TODO(Roskuski): This check might be detremental to the sound.
		NoteStateList[Note].State = PlayingSustained;
		// @TODO(Roskuski): I think that we're going to need to know the CurrentTime here to facilitate creating replay numbers.
		PlaySound(NoteSoundList[Note]);
	}
}

// Stops a sustained note
static inline void StopNoteSustained(note_name Note) {
	Assert(Note >= 0);
	Assert(Note < NoteName_Count);
	Assert(NoteStateList[Note].State == PlayingSustained);
	// @TODO(Roskuski): I think that we're going to need to know the CurrentTime here to facilitate creating replay numbers.
	StopSound(NoteSoundList[Note]);
	NoteStateList[Note].State = NotPlaying;
}

// Length is how long the note will play.
// Delay is how long we will wait until starting to play the note.
static inline void PlayNote(note_name Note, float CurrentTime, float Length, float Delay = 0.0f) {
	Assert(Note != NoteName_Count);
	if (NoteStateList[Note].State == NotPlaying) { // Right now, we will not attemt to replay a note if it is already playing, or currently playing.
		NoteStateList[Note].State = QueuedForPlaying;
		NoteStateList[Note].StartTime = CurrentTime + Delay;
		NoteStateList[Note].EndTime = CurrentTime + Delay + Length;
	}
}

static inline void StopNote(note_name Note, float CurrentTime) {
	Assert(Note >= 0);
	Assert(Note != NoteName_Count);
	NoteStateList[Note].EndTime = CurrentTime;
}

int main(void) {
	// Initialization
	//--------------------------------------------------------------------------------------
	const int screenWidth = 1280;
	const int screenHeight = 720;
	InitWindow(1280, 720, "Blues Bash");

	InitAudioDevice();

	const float BeatsPerMin = 120;
	const float BeatsPerSecond = BeatsPerMin / 60;
	const float SecondsPerBeat = 1 / BeatsPerSecond;
	printf("SpB: %f\n", SecondsPerBeat);

	// LoadAllNotes
	for (int Index = 0; Index < NoteName_Count; Index++) {
		NoteSoundList[Index] = LoadSound(NoteFileNames[Index]);
	}
	
	int Placement = 0;
	int LastKeyPressed = 0;
    int TempLastKey=0;
    int SecondLastKeyPressed;
  
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
            if (IsNotePlaying(Keyboard[Placement])) { Placement += 1; }
            Placement += 1;
			
			if (Placement >= 18) {
				Placement = 18;
			}
			
			PlayNoteSustained(Keyboard[Placement]);
            LastKeyPressed = 1;
			SustainedNotes[sustained_key::Right] = Keyboard[Placement];
            
		}
        
		if (IsKeyPressed(KEY_LEFT)) {
			
            if (IsNotePlaying(Keyboard[Placement])) { Placement -= 1; }
            Placement -= 1;
			
			if(Placement <= 0) {
				Placement = 0;
			}
            
			PlayNoteSustained(Keyboard[Placement]);
            LastKeyPressed = 0;
			SustainedNotes[sustained_key::Left] = Keyboard[Placement];
		}

		if (IsKeyPressed(KEY_DOWN)){
			PlayNoteSustained(Keyboard[Placement]);
			SustainedNotes[sustained_key::Down] = Keyboard[Placement];
		}	
		
		if(IsKeyPressed(KEY_UP)){
            
			if (LastKeyPressed == 0){
				
				Placement += 1;
				PlayNoteSustained(Keyboard[Placement]);
				SustainedNotes[sustained_key::Up] = Keyboard[Placement];
			}
			if (LastKeyPressed == 1){
				
				Placement -= 1;
				PlayNoteSustained(Keyboard[Placement]);
				SustainedNotes[sustained_key::Up] = Keyboard[Placement];
			}
            if (LastKeyPressed == 2){
				PlayNoteSustained(Keyboard[Placement]);
				SustainedNotes[sustained_key::Up] = Keyboard[Placement];
			}
            
            LastKeyPressed = 2;
            
            
		}

		for (int SustainedKey = 0; SustainedKey < SustainedKey_Count; SustainedKey++) {
			const KeyboardKey SusToRay[SustainedKey_Count] = {KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT};
			if (IsKeyReleased(SusToRay[SustainedKey])) {
				StopNoteSustained(SustainedNotes[SustainedKey]);
				SustainedNotes[SustainedKey] = NoteName_Count;
			}
		}
    
		for (int Index = 0; Index < NoteName_Count; Index++) {
			if (NoteStateList[Index].State == QueuedForPlaying) {
				if (NoteStateList[Index].StartTime <= CurrentTime &&
				    NoteStateList[Index].EndTime > CurrentTime) {
					NoteStateList[Index].State = Playing;
					PlaySound(NoteSoundList[Index]);
				}
			}
			else if (NoteStateList[Index].State == Playing) {
				if (NoteStateList[Index].EndTime <= CurrentTime) {
					StopSound(NoteSoundList[Index]);
					NoteStateList[Index].State = NotPlaying;
				}
			}
		}

		// Rendering
		{
			BeginDrawing();
			ClearBackground(RAYWHITE);
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
	CloseWindow();              // Close window and OpenGL context
	//--------------------------------------------------------------------------------------
    

	return 0;
}
