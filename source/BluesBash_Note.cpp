#include "BluesBash_Note.h"

bool IsNotePlaying(note_name Note, note_instrument Instrument) {
	bool Result = false;
	note_state_enum State = NoteStateList[Instrument][Note].State;
	
	switch(State) {
	case Playing:
	case PlayingSustained:
		Result = true;
	}

	return Result;
}

void PlayNoteSustained(note_name Note, note_instrument Instrument, float Volume) {
	Assert(Note >= 0);
	Assert(Note < NoteName_Count);
	if (!IsNotePlaying(Note, Instrument)) {
		// @TODO(Roskuski): I think that we're going to need to know the CurrentTime here to facilitate creating replay numbers. 
		NoteStateList[Instrument][Note].State = PlayingSustained;
		NoteStateList[Instrument][Note].Volume = Volume;
		NoteStateList[Instrument][Note].FadeRatio = 1;
		
		SetSoundVolume(NoteSoundList[Instrument][Note], Volume);
		PlaySound(NoteSoundList[Instrument][Note]);
	}
}

void StopNoteSustained(note_name Note, note_instrument Instrument) {
	Assert(Note >= 0);
	Assert(Note < NoteName_Count);
	
	// @TODO(Roskuski): I think that we're going to need to know the CurrentTime here to facilitate creating replay numbers.
	if (NoteStateList[Instrument][Note].State == PlayingSustained) {
		NoteStateList[Instrument][Note].State = Stopping;
	}
	else {
		printf("[WARNING]: We tried to stop a note that wasn't in PlayingSustained state! Doing nothing\n");
	}
}

void PlayNote(note_name Note, note_instrument Instrument, double CurrentTime, double Length, double Delay, float Volume) {
	Assert(Note != NoteName_Count);
	NoteStateList[Instrument][Note].State = QueuedForPlaying;
	NoteStateList[Instrument][Note].StartTime = CurrentTime + Delay;
	NoteStateList[Instrument][Note].EndTime = CurrentTime + Delay + Length;
	NoteStateList[Instrument][Note].Volume = Volume;
	NoteStateList[Instrument][Note].FadeRatio = 1;
}

void StopNote(note_name Note, note_instrument Instrument) {
	Assert(Note >= 0);
	Assert(Note != NoteName_Count);
	NoteStateList[Instrument][Note].State = Stopping;
}
