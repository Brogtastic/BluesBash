#include "BluesBash_Animation.h"

void LoadAnimationFromFiles(animation_enum Index, float FrameTime, int FrameCount, const char *PathFormatString) {
	animation * const Animation = &AnimationList[Index];
	Animation->FrameTime = FrameTime;
	Animation->FrameCount = FrameCount;

	Animation->Frames = (Texture2D*)malloc(FrameCount * sizeof(Texture2D));
	char *Buffer = (char*)malloc(sizeof(char) * 256); // @TODO(Roskuski): We should actually calculate a safe value instead of just shooting in the dark.
	
	for(int Index = 0; Index < FrameCount; Index++){
		sprintf(Buffer, PathFormatString, Index + 1);
		Animation->Frames[Index] = LoadTexture(Buffer);
	}
	free(Buffer);
}


Texture2D* GetCurrentFrame(animation_state State) {
	return &(AnimationList[State.Index].Frames[State.CurrentFrame]);
}

bool AnimateForwards(animation_state &State, float DeltaTime, bool Loop) {
	const animation &Animation = AnimationList[State.Index];
	bool Result = false;
	
	State.CurrentTime += DeltaTime;
	if (State.CurrentTime > Animation.FrameTime) {
		State.CurrentTime = 0;
		State.CurrentFrame += 1;
		if (State.CurrentFrame >= Animation.FrameCount) {
			Result = true;	
			if (Loop) {
				State.CurrentFrame = 0;
			}
			else {
				State.CurrentFrame = Animation.FrameCount - 1;
				State.CurrentTime = Animation.FrameTime;
			}
		}
	}

	return Result;
}

bool AnimateBackwards(animation_state &State, float DeltaTime, bool Loop) {
	const animation &Animation = AnimationList[State.Index];
	bool Result = false;
	
	State.CurrentTime -= DeltaTime;
	if (State.CurrentTime < 0) {
		State.CurrentTime = Animation.FrameTime;
		State.CurrentFrame -= 1;
		if (State.CurrentFrame < 0) {
			Result = true;
			if (Loop) {
				State.CurrentFrame = Animation.FrameCount - 1;
			}
			else {
				State.CurrentFrame = 0;
			}
		}
	}

	return Result;
}
