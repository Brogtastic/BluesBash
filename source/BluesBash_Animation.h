#pragma once
struct animation {
	double FrameTime;
	int FrameCount;
	Texture2D *Frames;
};

// NOTE(Roskuski): Used as a index into AnimationList
enum animation_enum {
	PlayButton,
	ListenButton,
	SettingsButton,
	TopMenuLight,
	
	AnimationEnum_Count, // NOTE(Roskuski): Keep this at the end.
};

global_var animation AnimationList[AnimationEnum_Count];

// Use animation_state to keep track of where you're currently at in a particular animation.
struct animation_state {
	animation_enum Index; // The animation that we are!
	int CurrentFrame; // The CurrentFrame we're on
	double CurrentTime; // How Long the we have been on the CurrentFrame.
};

// Index: an identifier in the animation_enum enumeration. If you're adding a new animation, add a corrisponding entry in animaion_enum in BluesBash_Animaion.h
// FrameTime: Time in seconds that a frame lasts for.
// FrameCount: The total number of frames in this animation
// PathFormatString: The Path to the animation files, with the "number" in the file name replaced with `%d`
void LoadAnimationFromFiles(animation_enum Index, double FrameTime, int FrameCount, const char *PathFormatString);

// Returns a pointer to a Texture2D that repersents the current frame of the animation.
Texture2D* GetCurrentFrame(animation_state State);

// State: the animation_state to do work on
// DeltaTime: How much time the last frame took
// Loop: If we should loop back to the first frame if we try to advance past the last frame
// Returns true we we just looped, or if we're at the end of our animation.
// Returns false otherwise.
bool AnimateForwards(animation_state &State, double DeltaTime, bool Loop);


// State: the animation_state to do work on
// DeltaTime: How much time the last frame took
// Loop: If we should loop back to the last frame if we try to regress past the first frame.
// Returns true we we just looped, or if we're at the first frame of our animation.
// Returns false otherwise.
bool AnimateBackwards(animation_state &State, double DeltaTime, bool Loop);
