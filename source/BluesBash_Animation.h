#pragma once
struct animation {
	float FrameTime;
	float CurrentTime;
	int CurrentFrame;
	int FrameCount;
	Texture2D *Frames;
};

enum animation_enum {
	PlayButton,
	ListenButton,
	SettingsButton,
	
	AnimationEnum_Count, // NOTE(Roskuski): Keep this at the end.
};

global_var animation AnimationList[AnimationEnum_Count];
