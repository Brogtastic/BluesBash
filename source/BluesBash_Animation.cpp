#include "BluesBash_Animation.h"
#include "BluesBash_Map.h"

// NOTE(Roskuski): animation.Frames and aniamtion.Frames[i].Graphic are both dynamically allocated
// NOTE(Roskuski): this function assumes that everything is well if it can successfully open the file.
void LoadAnimationFromFile(const char *Path) {
	animation Animation = {};
	FILE *AnimationFile = fopen(Path, "rb");
	if (AnimationFile != 0) {
		char *Key;
		int KeyLength;
		fread(&KeyLength, 4, 1, AnimationFile);
		Key = (char*)malloc(sizeof(char) * KeyLength + 1);
		Key[KeyLength] = 0;
		fread(Key, 1, KeyLength, AnimationFile);

		int FrameN, FrameD;
		fread(&FrameN, 4, 1, AnimationFile);
		fread(&FrameD, 4, 1, AnimationFile);
		Animation.FrameTime = ((double)(FrameN)) / ((double)(FrameD));

		fread(&Animation.UniqueFrameCount, 4, 1, AnimationFile);
		Animation.Frames = (frame*) malloc(sizeof(frame) * Animation.UniqueFrameCount);

		for (int FrameIndex = 0; FrameIndex < Animation.UniqueFrameCount; FrameIndex++) {
			int PathLen = 0;
			fread(&PathLen, 4, 1, AnimationFile);
			char *Path = (char*) malloc(PathLen + 1);
			Path[PathLen] = 0;
			{
				int ReadItems = fread(Path, 1, PathLen, AnimationFile);
				Assert(ReadItems == PathLen);
			}
			fread(&Animation.Frames[FrameIndex].FrameLength, 4, 1, AnimationFile);
			Animation.Frames[FrameIndex].Graphic = LoadTexture(Path);
			free(Path);
		}

		AnimationMap_Insert(Key, Animation);
		fclose(AnimationFile);
	}
	else {
		printf("[WARNING] LoadAnimationFromFile could not load *.ppp file: \"%s\"!\n", Path);
		Assert(false); // NOTE(Roskuski): Failed to load *.ppp file
	}
}

inline Texture2D* GetCurrentFrame(animation_state& State) {
	Texture2D *Result = 0;
	animation *Value = AnimationMap_Get(State.Key);
	if (Value) {
		Result = &Value->Frames[State.CurrentFrameMajor].Graphic;
	}
	return Result;
}

bool AnimateForwards(animation_state &State, double DeltaTime, bool Loop) {
	animation * const Animation = AnimationMap_Get(State.Key);
	bool Result = false;
	
	State.CurrentTime += DeltaTime;
	for (; State.CurrentTime > Animation->FrameTime; State.CurrentTime -= Animation->FrameTime) {
		State.CurrentFrameMinor += 1;

		if (State.CurrentFrameMinor > Animation->Frames[State.CurrentFrameMajor].FrameLength) {
			State.CurrentFrameMajor += 1;
			State.CurrentFrameMinor = 0;
			if (State.CurrentFrameMajor >= Animation->UniqueFrameCount) {
				Result = true;
				if (Loop) {
					State.CurrentFrameMajor = 0;
					State.CurrentFrameMinor = 0;
				}
				else { // Stay at limit
					State.CurrentFrameMajor = Animation->UniqueFrameCount - 1;
					State.CurrentFrameMinor = Animation->Frames[State.CurrentFrameMajor].FrameLength - 1;
					State.CurrentTime = Animation->FrameTime;
				}
			}
		}
	}

	return Result;
}

bool AnimateForwards(button_def *Button, double DeltaTime, bool Loop) {
	return AnimateForwards(Button->AniState, DeltaTime, Loop);
}

bool AnimateBackwards(animation_state &State, double DeltaTime, bool Loop) {
	animation * const Animation = AnimationMap_Get(State.Key);
	bool Result = false;
	
	State.CurrentTime -= DeltaTime;
	for (; State.CurrentTime < 0; State.CurrentTime += Animation->FrameTime) {
		State.CurrentFrameMinor -= 1;

		if (State.CurrentFrameMinor < 0) {
			State.CurrentFrameMajor -= 1;
			if (State.CurrentFrameMajor <= 0) {
				Result = true;
				if (Loop) {
					State.CurrentFrameMajor = Animation->UniqueFrameCount - 1;
					State.CurrentFrameMinor = Animation->Frames[State.CurrentFrameMajor].FrameLength - 1;
				}
				else { // Stay at limit
					State.CurrentFrameMajor = 0;
					State.CurrentFrameMinor = 0;
					State.CurrentTime = 0;
				}
			}
			else {
				State.CurrentFrameMinor = Animation->Frames[State.CurrentFrameMajor].FrameLength;
			}
		}
	}

	return Result;
}

bool AnimateBackwards(button_def *Button, double DeltaTime, bool Loop) {
	return AnimateBackwards(Button->AniState, DeltaTime, Loop);
}
