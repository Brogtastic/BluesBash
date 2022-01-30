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
#define NOTES	  \
	XX(A1, 0) \
		X(Bb1) \
		X(B1) \
		X(C1) \
		X(D1) \
		X(Eb1) \
		X(E1) \
		X(F1) \
		X(Fs1) \
		X(G1) \
		X(Gs1) \
		\
		X(Bb2) \
		X(C2) \
		X(Eb2) \
		X(F2) \
		X(Fs2) \
		X(G2) \
		\
		X(C3) \
		X(Bb3) \
		X(F3) \
		X(Fs3) \
		X(G3) \
		\
		X(Bb4) \
		X(C4) \
		X(Eb4) \
		X(F4) \
		X(Fs4) \
		X(G4) \
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

Sound NoteSoundList[NoteName_Count]; // @TODO(Roskuski): I think this could use a better name.

static inline void PlayNote(note_name Note) {
	Assert(Note != NoteName_Count);
	PlaySound(NoteSoundList[Note]);
}

static inline void StopNote(note_name Note) {
	Assert(Note != NoteName_Count);
	StopSound(NoteSoundList[Note]);
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
  
	note_name Keyboard[3] = {C1, Eb1, G1};
    
	while(!WindowShouldClose()) {
        
		if (IsKeyPressed(KEY_F)) {
			if((Placement == 0) || (Placement == 1)) {
				Placement = Placement + 1;
			}
			PlayNote(Keyboard[Placement]);
		}
        
        
		if (IsKeyPressed(KEY_D)) {
			if((Placement == 1) || (Placement == 2)) {
				Placement = Placement - 1;
			}
			PlayNote(Keyboard[Placement]);
		}
        
		if(IsKeyReleased(KEY_F)) {
			StopNote(Keyboard[Placement]);
		}            
        
		if(IsKeyReleased(KEY_D)) {
			StopNote(Keyboard[Placement]);
		}
        
		if((IsKeyUp(KEY_F)) && (IsKeyUp(KEY_D))) {
			StopNote(Keyboard[0]);
			StopNote(Keyboard[1]);
			StopNote(Keyboard[2]);
		}

		// Rendering
		{
			BeginDrawing();
			ClearBackground(RAYWHITE);
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
