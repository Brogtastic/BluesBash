#pragma once
struct animation {
	float FrameTime;
	int FrameCount;
	Texture2D *Frames;
};

// NOTE(Roskuski): Used as a index into AnimationList
enum animation_enum {
	PlayButton,
	ListenButton,
	SettingsButton,
	
	AnimationEnum_Count, // NOTE(Roskuski): Keep this at the end.
};

global_var animation AnimationList[AnimationEnum_Count];

struct animation_state {
	animation_enum Index;
	int CurrentFrame;
	float CurrentTime;
};
