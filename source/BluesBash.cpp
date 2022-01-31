#include <raylib.h>

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

enum note_state_enum {
	NotPlaying = 0,
	Playing,
};

struct note_state {
	note_state_enum State;
	float StartTime;
	float EndTime;
};

note_state NoteStateList[NoteName_Count];
Sound NoteSoundList[NoteName_Count];

// Length is how long the note will play.
// Delay is how long we will wait until starting to play the note.
static inline void PlayNote(note_name Note, float CurrentTime, float Length, float Delay = 0.0f) {
	Assert(Note != NoteName_Count);
	NoteStateList[Note].StartTime = CurrentTime + Delay;
	NoteStateList[Note].EndTime = CurrentTime + Delay + Length;
}

static inline void StopNote(note_name Note, float CurrentTime) {
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

	// LoadAllNotes
	for (int Index = 0; Index < NoteName_Count; Index++) {
		NoteSoundList[Index] = LoadSound(NoteFileNames[Index]);
	}
	
	int Placement = 0;
  
	note_name Keyboard[19] = {C2, Eb2, F2, Fs2, G2, Bb2, C3, Eb3, F3, Fs3, G3, Bb3, C4, Eb4, F4, Fs4, G4, Bb4, C5};
    
	while(!WindowShouldClose()) {
		float CurrentTime = GetTime();
		float DeltaTime = (float)GetFrameTime();

		
		if (IsKeyPressed(KEY_RIGHT)) {
			if (NoteStateList[Keyboard[Placement]].State == Playing) { Placement += 1; }
			Placement += 1;
			if (Placement >= 18) {
				Placement = 18;
			}
			
			PlayNote(Keyboard[Placement], CurrentTime, 0.25);
		}
        
        
		if (IsKeyPressed(KEY_LEFT)) {
			if (NoteStateList[Keyboard[Placement]].State == Playing) { Placement -= 1; }
			Placement -= 1;
			if(Placement <= 0) {
				Placement = 0;
			}
			
			PlayNote(Keyboard[Placement], CurrentTime, 0.25);
		}
        
		if (IsKeyPressed(KEY_DOWN)){
			PlayNote(Keyboard[Placement], CurrentTime, 0.25);
		}
    
		for (int Index = 0; Index < NoteName_Count; Index++) {
			if (NoteStateList[Index].State == NotPlaying) {
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
				DrawRectangleRec(Rect, NoteState.State == Playing ? GREEN : RED);
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
