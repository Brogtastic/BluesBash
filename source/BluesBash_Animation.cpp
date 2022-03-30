#include "BluesBash_Animation.h"

// Super basic string hashing
int StringHash(const char* String) {
	int Hash = 0;
	for (int Index = 0; String[Index] != 0; Index++) {
		Hash += String[Index];
	}
	return Hash;
}

void InitAnimationMap() {
	for (int Index = 0; Index < AnimationMap_BucketCount; Index++) {
		AnimationMap_Buckets[Index] = AnimationMap_NoEntry;
	}

	for (int Index = 0; Index < AnimationMap_BackingCount; Index++) {
		AnimationMap_BackingData[Index].NextIndex = Index + 1;
	}
	AnimationMap_BackingData[AnimationMap_BackingCount - 1].NextIndex = AnimationMap_NoEntry;

	AnimationMap_FreeListHead = 0;
}

int AnimationMap_GetFromFreeList() { 
	int Result = AnimationMap_NoEntry;
	if (AnimationMap_FreeListHead != AnimationMap_NoEntry) {
		Result = AnimationMap_FreeListHead;
		AnimationMap_FreeListHead = AnimationMap_BackingData[AnimationMap_FreeListHead].NextIndex;
	}

	AnimationMap_BackingData[Result].NextIndex = AnimationMap_NoEntry;

	return Result;
}

animation* AnimationMap_Get(const char *Key) {
	int Result = AnimationMap_NoEntry;

	int Hash = StringHash(Key);
	Result = AnimationMap_Buckets[Hash % AnimationMap_BucketCount];
	if (Result != AnimationMap_NoEntry) {
		animation_map_entry *Entry = &AnimationMap_BackingData[Result];
		while (Entry->NextIndex != AnimationMap_NoEntry) {
			if (strcmp(Key, Entry->Key) != 0) {
				Entry = &AnimationMap_BackingData[Entry->NextIndex];
			}
			else {
				break;
			}
		}
	}

	if (Result == AnimationMap_NoEntry) { 
		Assert(false); // This really shouldn't happen
		return 0;
	}
	return &(AnimationMap_BackingData[Result].Value);
}

// NOTE(Roskuski): Here, the data Key points to should remain valid until this entry is removed from the map.
void AnimationMap_Insert(char *Key, animation Value) {
	int Hash = StringHash(Key);
	int BucketIndex = Hash % AnimationMap_BucketCount;
	int EntryIndex = AnimationMap_Buckets[BucketIndex];

	int FreeIndex = AnimationMap_GetFromFreeList();
	Assert(FreeIndex != AnimationMap_NoEntry); // NOTE(Roskuski): If this assert fires, then we ran out of AnimationEntries. Bump up AnimationMap_BackingCount to a higher number.

	if (EntryIndex == AnimationMap_NoEntry) {
		AnimationMap_Buckets[BucketIndex] = FreeIndex;
	}
	else {
		animation_map_entry *ParentEntry = &AnimationMap_BackingData[EntryIndex];
		while (ParentEntry->NextIndex != AnimationMap_NoEntry) {
			ParentEntry = &AnimationMap_BackingData[ParentEntry->NextIndex];
		}
		ParentEntry->NextIndex = FreeIndex;
	}
	animation_map_entry &Entry = AnimationMap_BackingData[FreeIndex];
	Entry.Key = Key;
	Entry.Value = Value;
	Entry.NextIndex = AnimationMap_NoEntry;
}

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
	return &(AnimationMap_Get(State.Key)->Frames[State.CurrentFrameMajor].Graphic);
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
