
#define Assert(Cnd) if (!(Cnd)) { __debugbreak(); }
#define ArrayCount(Array) (sizeof(Array)/sizeof(Array[0]))

#define local_persist static

#define translation_scope static
#define global_var static

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstdio>
#include <cstdlib>

#pragma comment(lib, "Ws2_32.lib")


// NOTE(Roskuski) This is copied from BluesBash_Animation.h I cannot include the actual file because it depends on raylib.h
// raylib.h is incompatible with Windows.h
void LoadAnimationFromFile(const char *Path);
// NOTE(Roskuski) Similar reasoning as above...
void LoadUim(const char *Path);

#include "Server/Commands.h"

global_var addrinfo *AddressInfo;

net_server_nugget* ClientRequest(net_client_nugget *ClientNugget) {
	net_server_nugget *Result = 0;

	SOCKET ServerSock = socket(AddressInfo->ai_family, AddressInfo->ai_socktype, AddressInfo->ai_protocol);
	if (ServerSock != SOCKET_ERROR) {
		int ConnectResult = connect(ServerSock, AddressInfo->ai_addr, AddressInfo->ai_addrlen);
		if (ConnectResult != SOCKET_ERROR) {
			int SentBytes = send(ServerSock, (char*)ClientNugget, sizeof(*ClientNugget), 0);
			if (SentBytes != sizeof(*ClientNugget)) {
				printf("[ClientRequest]: Failed to send. Error Code %d.\n", WSAGetLastError());
			}

			Result = (net_server_nugget*)malloc(sizeof(net_server_nugget));

			int RecevedBytes = recv(ServerSock, (char*)Result, sizeof(*Result), 0);
			if (RecevedBytes != sizeof(*Result)) {
				printf("[ClientRequest]: Failed to Recveve. Error Code %d.\n", WSAGetLastError());
			}

			closesocket(ServerSock);
		}
		else { printf("[ClientRequest]: Failed to Connect. Error Code %d.\n", WSAGetLastError()); }
	}
	else { printf("[ClientRequest]: Failed to Open Socket. Error Code %d.\n", WSAGetLastError()); }

	return Result;
}

bool InitNetwork() {
	bool Success = true;
	WSADATA WinsockData = {};
	int ResultWinsockStartup = WSAStartup(MAKEWORD(2,2), &WinsockData);
	if (ResultWinsockStartup == 0) {
		addrinfo AddressInfoHint = {};
		AddressInfoHint.ai_family = AF_INET6;
		AddressInfoHint.ai_socktype = SOCK_STREAM;
		AddressInfoHint.ai_protocol = IPPROTO_TCP;

		int GetAdderInfoResult = getaddrinfo("localhost", ListenPort, &AddressInfoHint, &AddressInfo);
		if (GetAdderInfoResult == 0) {
		}
		else { printf("[main] getaddrinfo failed!\n"); Success = false; }
	}
	else { printf("[main] WinSock failed to init!\n"); Success = false; }
	return Success;
}

void UninitNetwork() {
	WSACleanup();
}

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
