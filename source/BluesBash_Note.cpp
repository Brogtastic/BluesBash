#include "BluesBash_Note.h"

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
