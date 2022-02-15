#include "BluesBash_Note.h"

bool IsNotePlaying(note_name Note) {
	bool Result = false;
	note_state_enum State = NoteStateList[Note].State;
	
	switch(State) {
	case Playing:
	case PlayingSustained:
		Result = true;
	}

	return Result;
}

void PlayNoteSustained(note_name Note, float Volume) {
	Assert(Note >= 0);
	Assert(Note < NoteName_Count);
	if (NoteStateList[Note].State == NotPlaying) { // @TODO(Roskuski): This check might be detremental to the sound.
		// @TODO(Roskuski): I think that we're going to need to know the CurrentTime here to facilitate creating replay numbers. 
		NoteStateList[Note].State = PlayingSustained;
		NoteStateList[Note].Volume = Volume;
		
		SetSoundVolume(NoteSoundList[Note], Volume);
		PlaySound(NoteSoundList[Note]);
	}
}

void StopNoteSustained(note_name Note) {
	Assert(Note >= 0);
	Assert(Note < NoteName_Count);
	
	// @TODO(Roskuski): I think that we're going to need to know the CurrentTime here to facilitate creating replay numbers.
	if (NoteStateList[Note].State == PlayingSustained) {
		StopSound(NoteSoundList[Note]);
		SetSoundVolume(NoteSoundList[Note], 1.0);
		
		NoteStateList[Note].Volume = 1.0;
		NoteStateList[Note].State = NotPlaying;
	}
	else {
		printf("[WARNING]: We tried to stop a note that wasn't in PlayingSustained state! Doing nothing\n");
	}
}

void PlayNote(note_name Note, float CurrentTime, float Length, float Delay, float Volume) {
	Assert(Note != NoteName_Count);
	NoteStateList[Note].State = QueuedForPlaying;
	NoteStateList[Note].StartTime = CurrentTime + Delay;
	NoteStateList[Note].EndTime = CurrentTime + Delay + Length;
	NoteStateList[Note].Volume = Volume;
}

void StopNote(note_name Note, float CurrentTime) {
	Assert(Note >= 0);
	Assert(Note != NoteName_Count);
	NoteStateList[Note].EndTime = CurrentTime;
}
