#include "BluesBash_Map.h"

// Super basic string hashing
int StringHash(const char* String) {
	int Hash = 0;
	for (int Index = 0; String[Index] != 0; Index++) {
		Hash += String[Index];
	}
	return Hash;
}

// Init's map freelist
void MAP_Init(const int BucketCount, int *Buckets, int BackingCount, size_t BackingObjectSize, uintptr_t BackingNextIndexOffset, char *Backing, int *FreeListHead) {
	for (int Index = 0; Index < BucketCount; Index++) {
		Buckets[Index] = MAP_NoEntry;
	}

	for (int Index = 0; Index < BackingCount; Index++) {
		int *Temp = (int*)ManualArrayIndexMember(Backing, Index, BackingObjectSize, BackingNextIndexOffset);
		*Temp = Index + 1;
	}
		*((int*)ManualArrayIndexMember(Backing, BackingCount - 1, BackingObjectSize, BackingNextIndexOffset)) = MAP_NoEntry;

	*FreeListHead = 0;
}

int MAP_GetFromFreeList(int *FreeListHead, size_t BackingObjectSize, uintptr_t BackingNextIndexOffset, char *Backing) {
	int Result = MAP_NoEntry;
	if (*FreeListHead != MAP_NoEntry) {
		Result = *FreeListHead; // Index => NextIndex
		*FreeListHead = *((int*)ManualArrayIndexMember(Backing, Result, BackingObjectSize, BackingNextIndexOffset));
	}

	if (Result != MAP_NoEntry) {
		*((int*)ManualArrayIndexMember(Backing, Result, BackingObjectSize, BackingNextIndexOffset)) = MAP_NoEntry;
	}
	else {
		Assert(false); // NOTE(Roskuski): The freelist is empty! just increase the size of the backing data!
	}

	// NOTE(Roskuski): Should only get here with a valid result.
	return Result;
}

void MAP_Insert(char *Key, void *Value, size_t ValueSize, uintptr_t ValueOffset, int *FreeListHead, int BucketCount, int *Buckets, size_t BackingObjectSize, uintptr_t BackingKeyOffset, uintptr_t BackingNextIndexOffset, char *Backing) {
	int Hash = StringHash(Key);
	int BucketIndex = Hash % BucketCount;
	int EntryIndex = Buckets[BucketIndex];

	int FreeIndex = MAP_GetFromFreeList(FreeListHead, BackingObjectSize, BackingNextIndexOffset, Backing);
	Assert(FreeIndex != MAP_NoEntry);

	if (EntryIndex == MAP_NoEntry) {
		Buckets[BucketIndex] = FreeIndex;
	}
	else {
		char *ParentEntry = (char*)(ManualArrayIndexMember(Backing, EntryIndex, BackingObjectSize, 0));
		int *ParentEntry_NextIndex = ((int*)(ParentEntry + BackingNextIndexOffset));
		while (*ParentEntry_NextIndex != MAP_NoEntry) {
			ParentEntry = ManualArrayIndexMember(Backing, *ParentEntry_NextIndex, BackingObjectSize, 0);
			ParentEntry_NextIndex = ((int*)(ParentEntry + BackingNextIndexOffset));
		}
		*ParentEntry_NextIndex = FreeIndex;
	}
	*((char**)ManualArrayIndexMember(Backing, FreeIndex, BackingObjectSize, BackingKeyOffset)) = Key;
	*((int*)ManualArrayIndexMember(Backing, FreeIndex, BackingObjectSize, BackingNextIndexOffset)) = MAP_NoEntry;
	memcpy(ManualArrayIndexMember(Backing, FreeIndex, BackingObjectSize, ValueOffset), ((char*)Value) + ValueOffset, ValueSize);
}

void* MAP_Get(const char *Key, int BucketCount, int *Buckets, size_t BackingObjectSize, uintptr_t BackingKeyOffset, uintptr_t BackingNextIndexOffset, char *Backing) {
	int Hash = StringHash(Key);
	int Result = Buckets[Hash % BucketCount];
	char *Entry = 0;

	if (Result != MAP_NoEntry) {
		Entry = ((char*)ManualArrayIndexMember(Backing, Result, BackingObjectSize, 0));
		int Entry_NextIndex = *((int*)(Entry + BackingNextIndexOffset));
		char **Entry_Key = ((char**)(Entry + BackingKeyOffset));

		while (Entry_NextIndex != MAP_NoEntry) {
			if (strcmp(Key, *Entry_Key) == 0) {
				break; // Found it!
			}
			else {
				if (Entry_NextIndex != MAP_NoEntry) {
					Entry = ((char*)ManualArrayIndexMember(Backing, BackingObjectSize, Entry_NextIndex, 0));
				}
				else {
					Entry = 0;
					break; // Key is not in map
				}
			}
		}
	}

	return Entry;
}


