#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstdio>

#pragma comment(lib, "Ws2_32.lib")

#define Assert(Cnd) if (!(Cnd)) { __debugbreak(); }

// Used for static locals.
#define local_persist static

// Used for static globals/functions
#define translation_scope static
#define global_var static

#include "Commands.h"

SOCKET ListenSocket;

void HandleConnection(SOCKET ClientSock) {
	net_client_nugget ClientNugget;
	recv(ClientSock, (char*)(&ClientNugget), sizeof(ClientNugget), 0);

	printf("[HandleConnection]: Handling ClientNugget (%d)\n", ClientNugget.Command);

	switch (ClientNugget.Command) {
		case Net_Client_Logon: {
			// @TODO(Roskuski): This is placeholder:
			printf("Email: \"%s\", Password: \"%s\"\n", ClientNugget.Data.Logon.Email, ClientNugget.Data.Logon.Password);
			net_server_nugget Responce = {};
			Responce.Command = Net_Server_LogonOk;
			memcpy(Responce.Data.LogonOk.Nickname, "Placeholder Name", strlen("Placeholder Name"));
			Responce.Data.LogonOk.UserId = 101;
			send(ClientSock, (char*)(&Responce), sizeof(Responce), 0);
		} break;
		case Net_Client_SecurityQuestion: {
			
		} break;
		default: {
			printf("[HandleConnection]: A Client nugget had an invalid command (%d)!\n", ClientNugget.Command);
		}
	}

	shutdown(ClientSock, SD_SEND);
	closesocket(ClientSock);
}

int main() {
	WSADATA WinsockData = {};
	int ResultWinsockStartup = WSAStartup(MAKEWORD(2,2), &WinsockData);
	if (ResultWinsockStartup == 0) {
		addrinfo *AddressInfo = {};
		addrinfo AddressInfoHint = {};
		AddressInfoHint.ai_family = AF_INET6;
		AddressInfoHint.ai_socktype = SOCK_STREAM;
		AddressInfoHint.ai_protocol = IPPROTO_TCP;
		AddressInfoHint.ai_flags = AI_PASSIVE;
		
		int GetAdderInfoResult = getaddrinfo("localhost", ListenPort, &AddressInfoHint, &AddressInfo);

		if (GetAdderInfoResult == 0) {
			ListenSocket = socket(AddressInfo->ai_family, AddressInfo->ai_socktype, AddressInfo->ai_protocol);

			if (ListenSocket != INVALID_SOCKET) {
				int BindResult = bind(ListenSocket, AddressInfo->ai_addr, (int)(AddressInfo->ai_addrlen));
				freeaddrinfo(AddressInfo);

				if (BindResult != SOCKET_ERROR) {
					if (listen(ListenSocket, SOMAXCONN) != SOCKET_ERROR) {
						while(true) {
							sockaddr OuterSockAddr = {};
							int OuterSockAddrLen = sizeof(sockaddr);
							printf("[main] Waiting for connection\n");
							SOCKET OuterSocket = accept(ListenSocket, 0, 0);
							// @TODO(Roskuski): Error Handling
							HandleConnection(OuterSocket);
						}
					}
					else { printf("[main] Failed to listen on ListenSocket!\n"); }
				}
				else { printf("[main] Failed to bind ListenSocket!\n"); }
				closesocket(ListenSocket);
			}
			else { printf("[main] Failed to make ListenSocket!\n"); }
		}
		else { printf("[main] Failed to Get Address Info!\n"); }
	}
	else { printf("[main] Failed to start Winsock2!\n"); }

	WSACleanup();
	return 0;
}
