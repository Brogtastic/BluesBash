#pragma once

struct frame {
	int FrameLength; // How many frames should we display this frame for
	Texture2D Graphic; // The actual image
};

struct animation {
	double FrameTime; // Frames per second
	int UniqueFrameCount; // How many unique frames are there
	frame *Frames; // Array of frame data, Length is the same as UniqueFrameCount
};

struct animation_map_entry {
	char *Key;
	animation Value;
	int NextIndex;
};

// Animation Hash Map Begins
#define AnimationMap_BucketCount (30)
#define AnimationMap_BackingCount (50)
#define AnimationMap_NoEntry (-1)

animation_map_entry AnimationMap_BackingData[AnimationMap_BackingCount];
int AnimationMap_FreeListHead = AnimationMap_NoEntry;
int AnimationMap_Buckets[AnimationMap_BucketCount];

// Inits AnimationMap_FreeListHead, as well as putting all entries into the freelist.
void InitAnimationMap();

// Animation Hash Map Ends

// Use animation_state to keep track of where you're currently at in a particular animation.
struct animation_state {
	const char *Key;
	// @TODO(Roskuski): I'd like to change these names from *Major/*Minor to something else
	int CurrentFrameMajor; // Current Frame Index
	int CurrentFrameMinor; // How many frames we've been on this frame.
	double CurrentTime; // How Long the we have been on the CurrentFrame.
};

void LoadAnimationFromFile(const char *Path);

// Returns a pointer to a Texture2D that repersents the current frame of the animation.
Texture2D* GetCurrentFrame(animation_state &State);

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
