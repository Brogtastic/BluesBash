#include <Windows.h>

// NOTE(Roskuski) This is copied from BluesBash_Animation.h I cannot include the actual file because it depends on raylib.h
// raylib.h is incompatible with Windows.h
void LoadAnimationFromFile(const char *Path);
// NOTE(Roskuski) Similar reasoning as above...
void LoadUim(const char *Path);

void LoadAllPppFiles() {
	// @TODO(Roskuski): this will likely crash if we don't have any ppp files in here. That said, we'd have bigger problems if that where the case!
	WIN32_FIND_DATA FindData = {};
	HANDLE FindHandle = FindFirstFile("resources/processed/*.ppp", &FindData);
	do {
		if ((strcmp(FindData.cFileName, ".")  == 0) ||
		    (strcmp(FindData.cFileName, "..") == 0)) {
			// NOTE(Roskuski) Skip...
		}
		else {
			int FindFileLen = strlen(FindData.cFileName);
			const char *Prefix = "resources/processed/";
			int PrefixLen = strlen(Prefix);
			char *TempPath = (char*)malloc(FindFileLen + PrefixLen + 1);
			TempPath[FindFileLen + PrefixLen] = 0;
			memcpy(TempPath, Prefix, PrefixLen);
			memcpy(TempPath + PrefixLen, FindData.cFileName, FindFileLen);
			LoadAnimationFromFile(TempPath);
			free(TempPath);
		}
	} while(FindNextFile(FindHandle, &FindData));
}

void LoadAllUimFiles() {
	// @TODO(Roskuski): this will likely crash if we don't have any ppp files in here. That said, we'd have bigger problems if that where the case!
	WIN32_FIND_DATA FindData = {};
	HANDLE FindHandle = FindFirstFile("resources/uim/*.uim", &FindData);
	do {
		if ((strcmp(FindData.cFileName, ".")  == 0) ||
		    (strcmp(FindData.cFileName, "..") == 0)) {
			// NOTE(Roskuski) Skip...
		}
		else {
			int FindFileLen = strlen(FindData.cFileName);
			const char *Prefix = "resources/uim/";
			int PrefixLen = strlen(Prefix);
			char *TempPath = (char*)malloc(FindFileLen + PrefixLen + 1);
			TempPath[FindFileLen + PrefixLen] = 0;
			memcpy(TempPath, Prefix, PrefixLen);
			memcpy(TempPath + PrefixLen, FindData.cFileName, FindFileLen);
			LoadUim(TempPath);
			free(TempPath);
		}
	} while(FindNextFile(FindHandle, &FindData));
}
