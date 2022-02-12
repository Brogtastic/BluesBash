#pragma once
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
#define XX(Name, Value) X(Name)
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


global_var note_state NoteStateList[NoteName_Count];
global_var Sound NoteSoundList[NoteName_Count];
