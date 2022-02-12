#include "BluesBash_Animation.h"

void LoadAnimationFromFiles(animation_enum Index, float FrameTime, int FrameCount, const char *PathFormatString) {
	animation * const Animation = &AnimationList[Index];
	Animation->FrameTime = FrameTime;
	Animation->FrameCount = FrameCount;
	Animation->CurrentFrame = 0;
	Animation->CurrentTime = 0;

	Animation->Frames = (Texture2D*)malloc(FrameCount * sizeof(Texture2D));
	char *Buffer = (char*)malloc(sizeof(char) * 256); // @TODO(Roskuski): We should actually calculate a safe value instead of just shooting in the dark.
	
	for(int Index = 0; Index < FrameCount; Index++){
		sprintf(Buffer, PathFormatString, Index + 1);
		Image Temp = LoadImage(Buffer);
		ImageResize(&Temp, 365, 205.35); // @TODO(Roskuski): We should change these magic numbes into named constants, Also why do these magic numbers have these values.
		Animation->Frames[Index] = LoadTextureFromImage(Temp);
		UnloadImage(Temp);
	}
	free(Buffer);
}

inline Texture2D* GetCurrentFrame(animation_enum Index) {
	return &AnimationList[Index].Frames[AnimationList[Index].CurrentFrame];
}
