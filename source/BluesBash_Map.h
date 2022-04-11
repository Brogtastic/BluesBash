#pragma once
struct map_entry {
	char *Key;
	int NextIndex;
};

#define OffsetOf(Thing, Type) ((uintptr_t)(&(((Type*)(0))->Thing)))

// NOTE(Rosksuski): Assumes that sizeof(Array) == 1.
// Used to access particular member of a struct that is in an array.
// Useful when you have the start of an array, and the offset of the member, but don't have type info for the array.
#define ManualArrayIndexMember(Array, Index, StructureSize, MemberOffset) \
	((Array) + ((Index) * (StructureSize)) + (MemberOffset))

const int MAP_NoEntry = -1;

int StringHash(const char *String);

// NOTE(Roskuski): How to use this string hash map.
// To make a hashmap for a given datatype, you must do the following
//  In the datatype, have a map_entry called Entry as the first element.
//  define an array to use as backing data, with it's length as an accessable value.
//  define an array of ints to hold the buckets, with it's length as an accessable value.
//  define a int to use as the free list head.
// example follows
/*
struct animation {
	map_entry Entry;
	// The struct's actual data follows
	double FrameTime; // Frames per second
	int UniqueFrameCount; // How many unique frames are there
	frame *Frames; // Array of frame data, Length is the same as UniqueFrameCount
};
#define AnimationMap_BucketCount (30)
#define AnimationMap_BackingCount (50)
#define AnimationMap_NoEntry (-1)

animation AnimationMap_BackingData[AnimationMap_BackingCount];
int AnimationMap_FreeListHead = AnimationMap_NoEntry;
int AnimationMap_Buckets[AnimationMap_BucketCount];
*/
// To make using maps easier, create a macro for the map type that fills out the parameters for you.


// Sets up Bucket data and Freelist.
void MAP_Init(const int BucketCount, int *Buckets, int BackingCount, size_t BackingObjectSize, uintptr_t BackingNextIndexOffset, char *Backing, int *FreeListHead);
#define AnimationMap_Init() \
	MAP_Init(AnimationMap_BucketCount, AnimationMap_Buckets, AnimationMap_BackingCount, sizeof(animation), OffsetOf(Entry.NextIndex, animation), (char*)(&AnimationMap_BackingData[0]), &AnimationMap_FreeListHead)
#define ButtonMap_Init() \
	MAP_Init(ButtonMap_BucketCount, ButtonMap_Buckets, ButtonMap_BackingCount, sizeof(button_def), OffsetOf(Entry.NextIndex, button_def), (char*)(&ButtonMap_Backing[0]), &ButtonMap_FreeListHead)

// Mainly for internal use. Pops a entry off the free list.
int MAP_GetFromFreeList(int *FreeListHead, size_t BackingObjectSize, uintptr_t BackingNextIndexOffset, char *Backing);

// Inserts a Value at Key.
// ValueSize = sizeof(Value's type) - sizeof(map_entry)
// ValueOffset = OffsetOf(Value's first actual data entry, Value's type)
void MAP_Insert(char *Key, void *Value, size_t ValueSize, uintptr_t ValueOffset, int *FreeListHead, int BucketCount, int *Buckets, size_t BackingObjectSize, uintptr_t BackingKeyOffset, uintptr_t BackingNextIndexOffset, char *Backing);
#define AnimationMap_Insert(KEY, VALUE) \
	MAP_Insert((KEY), &(VALUE), sizeof(animation) - sizeof(map_entry), OffsetOf(FrameTime, animation), &AnimationMap_FreeListHead, AnimationMap_BucketCount, AnimationMap_Buckets, sizeof(animation), OffsetOf(Entry.Key, animation), OffsetOf(Entry.NextIndex, animation), (char*)(&AnimationMap_BackingData[0]))
#define ButtonMap_Insert(KEY, VALUE) \
	MAP_Insert((KEY), &(VALUE), sizeof(button_def) - sizeof(map_entry), OffsetOf(HitRect, button_def), &ButtonMap_FreeListHead, ButtonMap_BucketCount, ButtonMap_Buckets, sizeof(button_def), OffsetOf(Entry.Key, button_def), OffsetOf(Entry.NextIndex, button_def), (char*)(&ButtonMap_Backing[0]))

// Gets the Value at Key.
void* MAP_Get(const char *Key, int BucketCount, int *Buckets, size_t BackingObjectSize, uintptr_t BackingKeyOffset, uintptr_t BackingNextIndexOffset, char *Backing);
#define AnimationMap_Get(KEY) \
	((animation*)MAP_Get((KEY), AnimationMap_BucketCount, AnimationMap_Buckets, sizeof(animation), OffsetOf(Entry.Key, animation), OffsetOf(Entry.NextIndex, animation), (char*)(&AnimationMap_BackingData[0])))
#define ButtonMap_Get(KEY) \
	((button_def*)MAP_Get((KEY), ButtonMap_BucketCount, ButtonMap_Buckets, sizeof(button_def), OffsetOf(Entry.Key, button_def), OffsetOf(Entry.NextIndex, button_def), (char*)(&ButtonMap_Backing[0])))
