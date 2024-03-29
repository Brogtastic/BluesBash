#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstdio>
#include <sqlite3.h>

#pragma comment(lib, "Ws2_32.lib")

#define Assert(Cnd) if (!(Cnd)) { __debugbreak(); }

// Used for static locals.
#define local_persist static

// Used for static globals/functions
#define translation_scope static
#define global_var static

#include "Commands.h"

const char *DatabaseCreationSQL =
"CREATE TABLE Users (\n"
    "Id          INTEGER       PRIMARY KEY AUTOINCREMENT\n"
                              "NOT NULL\n"
                              "UNIQUE,\n"
    "Nickname    VARCHAR (45)  NOT NULL\n"
                              "UNIQUE,\n"
    "Email       VARCHAR (100) UNIQUE\n"
                              "NOT NULL,\n"
    "Password    VARCHAR (45)  NOT NULL,\n"
    "SecQuestion VARCHAR (100) NOT NULL,\n"
    "SecAnswer   VARCHAR (100) NOT NULL\n"
");\n"
"\n"
"CREATE TABLE Tracks (\n"
    "Id     INTEGER      PRIMARY KEY AUTOINCREMENT\n"
                        "UNIQUE\n"
                        "NOT NULL,\n"
    "Name   VARCHAR (45) NOT NULL,\n"
    "Author INTEGER      REFERENCES Users (Id) \n"
                        "NOT NULL,\n"
    "Parent INTEGER      REFERENCES Tracks (Id) \n"
");\n"
"\n"
"CREATE TABLE Comments (\n"
    "Id          INTEGER       PRIMARY KEY AUTOINCREMENT\n"
                              "UNIQUE\n"
                              "NOT NULL,\n"
    "Who         INTEGER       REFERENCES Users (Id) \n"
                              "NOT NULL,\n"
    "What        INTEGER       REFERENCES Tracks (Id) \n"
                              "NOT NULL,\n"
    "TimeCreated TIME          NOT NULL,\n"
    "Data        VARCHAR (300) NOT NULL\n"
");\n"
"\n"
"CREATE TABLE User_Ratings (\n"
    "Id     INTEGER PRIMARY KEY AUTOINCREMENT\n"
                   "NOT NULL\n"
                   "UNIQUE,\n"
    "Who    INTEGER REFERENCES Users (Id) \n"
                   "NOT NULL,\n"
    "What   INTEGER REFERENCES Tracks (Id) \n"
                   "NOT NULL,\n"
    "Rating INTEGER NOT NULL\n"
");\n";

char QueryBuffer[2000] = {}; // @TODO(Roskuski): This is probably wastful

const char *RegisterTestEmailQuery = "SELECT Email FROM Users WHERE Email = \'%s\';";
const char *RegisterTestNicknameQuery = "SELECT Nickname FROM Users WHERE Nickname = \'%s\';";
const char *RegisterQuery = "INSERT INTO Users (Email, Password, Nickname, SecQuestion, SecAnswer) VALUES (\'%s\', \'%s\', \'%s\', \'%s\', \'%s\');";
const char *PostRegisterQuery = "SELECT Id, Nickname FROM Users WHERE Email = \'%s\';";
const char *LogonQuery = "SELECT Id, Nickname FROM Users WHERE Email = \'%s\' AND Password = \'%s\';";

SOCKET ListenSocket;
sqlite3 *Database;
#define DatabaseName "BluesBash_Database.db"

char* Sanitize(char *String) {
	char *Result;
	int CountOfSingleQuotes = 0;
	int StringLength = strlen(String);

	for (int Index = 0; Index < StringLength; Index++) {
		if (String[Index] == '\'') { CountOfSingleQuotes++; }
	}
	Result = (char*)malloc(StringLength + CountOfSingleQuotes + 1);
	Result[StringLength + CountOfSingleQuotes] = 0;

	int WriteIndex = 0;
	for (int Index = 0; Index < StringLength; Index++) {
		Result[WriteIndex] = String[Index];
		WriteIndex++;
		if (String[Index] == '\'') {
			Result[WriteIndex] = '\'';
			WriteIndex++;
		}
	}

	return Result; 
}

void ClientLogon(net_client_nugget ClientNugget, SOCKET ClientSock) {
	net_server_nugget Reply = {};
	net_client_logon_def *Data = &ClientNugget.Data.Logon;

	char *SEmail = Sanitize(Data->Email);
	char *SPassword = Sanitize(Data->Password);

	snprintf(QueryBuffer, 2000, LogonQuery, SEmail, SPassword);
	sqlite3_stmt *Statement;
	int PrepareResult = sqlite3_prepare_v2(Database, QueryBuffer, -1, &Statement, 0);
	if (PrepareResult != SQLITE_OK) {
		printf("prepare ClientLogonQuery: %d %s\n", PrepareResult, sqlite3_errmsg(Database));
	}
	else {
		int StepResult = sqlite3_step(Statement);
		if (StepResult == SQLITE_DONE) {
			// We found nothing
			Reply.Command = Net_Server_LogonFail;
		}
		else if (StepResult == SQLITE_ROW) {
			// We found something
			Reply.Command = Net_Server_LogonOk;
			Reply.Data.LogonOk.UserId = sqlite3_column_int(Statement, 0);
			const unsigned char *TableNickname = sqlite3_column_text(Statement, 1);
			memcpy(Reply.Data.LogonOk.Nickname, TableNickname, NICKNAME_LEN);
			for (StepResult = sqlite3_step(Statement); StepResult != SQLITE_DONE; StepResult = sqlite3_step(Statement)) {}
		}
	}

	send(ClientSock, (char*)&Reply, sizeof(Reply), 0);
	free(SEmail);
	free(SPassword);
	return;
}

void ClientRegister(net_client_nugget ClientNugget, SOCKET ClientSock) {
	net_server_nugget Reply = {};
	Reply.Command = Net_Server_RegisterOk;
	net_client_register_def *Data = &ClientNugget.Data.Register;
	char *SEmail = Sanitize(Data->Email);
	char *SPassword = Sanitize(Data->Password);
	char *SNickname = Sanitize(Data->Nickname);
	char *SSecQuestion = Sanitize(Data->SecQuestion);
	char *SSecAnswer = Sanitize(Data->SecAnswer);

	if (strlen(SSecAnswer) == 0) {
		Reply.Command = Net_Server_RegisterFail;
		Reply.Data.RegisterFail.Reason = Net_Server_RegisterFail_EmptySecAnswer;
	}
	if (strlen(SSecQuestion) == 0) {
		Reply.Command = Net_Server_RegisterFail;
		Reply.Data.RegisterFail.Reason = Net_Server_RegisterFail_EmptySecQuestion;
	}
	if (strlen(SNickname) == 0) {
		Reply.Command = Net_Server_RegisterFail;
		Reply.Data.RegisterFail.Reason = Net_Server_RegisterFail_EmptyNickname;
	}
	if (strlen(SPassword) == 0) {
		Reply.Command = Net_Server_RegisterFail;
		Reply.Data.RegisterFail.Reason = Net_Server_RegisterFail_EmptyPassword;
	}
	if (strlen(SEmail) == 0) {
		Reply.Command = Net_Server_RegisterFail;
		Reply.Data.RegisterFail.Reason = Net_Server_RegisterFail_EmptyEmail;
	}

	{
		snprintf(QueryBuffer, 2000, RegisterTestEmailQuery, SEmail);
		sqlite3_stmt *Statement;
		sqlite3_prepare_v2(Database, QueryBuffer, -1, &Statement, 0);
		

		int StepResult = sqlite3_step(Statement);
		if (StepResult == SQLITE_DONE) {
		}
		else if (StepResult == SQLITE_ROW) {
			Reply.Command = Net_Server_RegisterFail;
			Reply.Data.RegisterFail.Reason = Net_Server_RegisterFail_UsedEmail;
		}
		sqlite3_finalize(Statement);
	}

	{
		snprintf(QueryBuffer, 2000, RegisterTestNicknameQuery, SNickname);
		sqlite3_stmt *Statement;
		sqlite3_prepare_v2(Database, QueryBuffer, -1, &Statement, 0);
		

		int StepResult = sqlite3_step(Statement);
		if (StepResult == SQLITE_DONE) {
		}
		else if (StepResult == SQLITE_ROW) {
			Reply.Command = Net_Server_RegisterFail;
			Reply.Data.RegisterFail.Reason = Net_Server_RegisterFail_UsedNickname;
		}
		sqlite3_finalize(Statement);
	}

	if (Reply.Command == Net_Server_RegisterOk) {
		snprintf(QueryBuffer, 2000, RegisterQuery, SEmail, SPassword, SNickname, SSecQuestion, SSecAnswer);
		char *ErrorMessage = 0;
		int ExecResult = sqlite3_exec(Database, QueryBuffer, 0, 0, &ErrorMessage); 
		if ((ExecResult != SQLITE_OK) && ErrorMessage) {
			printf("Net_Client_Register, RegisterQuery: %s\n", ErrorMessage);
			sqlite3_free(ErrorMessage);
			Reply.Command = Net_Server_RegisterFail;
			Reply.Data.RegisterFail.Reason = Net_Server_RegisterFail_Generic;
			sqlite3_free(ErrorMessage);
		}
		else {
			snprintf(QueryBuffer, 2000, PostRegisterQuery, SEmail);
			sqlite3_stmt *Statement;
			int PrepareResult = sqlite3_prepare_v2(Database, QueryBuffer, -1, &Statement, 0);
			if ((PrepareResult  != SQLITE_OK) && ErrorMessage) {
				printf("Net_Client_Register, PostRegisterQuery Prep: %d %s\n", PrepareResult, sqlite3_errmsg(Database));
				Reply.Command = Net_Server_RegisterFail;
				Reply.Data.RegisterFail.Reason = Net_Server_RegisterFail_Generic;
			}
			else {
				int StepResult = sqlite3_step(Statement);
				if (StepResult != SQLITE_ROW) {
					printf("Net_Client_Register, PostRegisterQuery Step Code: %d %s\n", StepResult, sqlite3_errmsg(Database));
					Reply.Command = Net_Server_RegisterFail;
					Reply.Data.RegisterFail.Reason = Net_Server_RegisterFail_Generic;
				}
				else {
					Reply.Data.RegisterOk.UserId = sqlite3_column_int(Statement, 0);
					const unsigned char *TableNickname = sqlite3_column_text(Statement, 1);
					memcpy(Reply.Data.RegisterOk.Nickname, TableNickname, NICKNAME_LEN);
					for (int StepResult = sqlite3_step(Statement); StepResult != SQLITE_DONE; StepResult = sqlite3_step(Statement)) {}
				}
			}
		sqlite3_finalize(Statement);
		}
	}

	free(SEmail);
	free(SPassword);
	free(SNickname);
	free(SSecQuestion);
	free(SSecAnswer);
	send(ClientSock, (char*)(&Reply), sizeof(Reply), 0);
	return;
}

void HandleConnection(SOCKET ClientSock) {
	sqlite3_open(DatabaseName, &Database);
	net_client_nugget ClientNugget;
	recv(ClientSock, (char*)(&ClientNugget), sizeof(ClientNugget), 0);

	printf("[HandleConnection]: Handling ClientNugget (%d)\n", ClientNugget.Command);

	switch (ClientNugget.Command) {
		case Net_Client_Logon: {
			ClientLogon(ClientNugget, ClientSock);
		} break;
		case Net_Client_Register: {
			ClientRegister(ClientNugget, ClientSock);
		} break;
		case Net_Client_SecurityQuestion: {
			
		} break;
		default: {
			printf("[HandleConnection]: A Client nugget had an invalid command (%d)!\n", ClientNugget.Command);
		}
	}

	shutdown(ClientSock, SD_SEND);
	closesocket(ClientSock);
	sqlite3_close(Database);
}

int main() {
	{
		DWORD Attributes = GetFileAttributes(DatabaseName);
		if (Attributes == INVALID_FILE_ATTRIBUTES) {
			printf("[main]: Did not find database, populating a new one.");
			// NOTE(Roskuski): File does not exist, time to make it.
			sqlite3_open(DatabaseName, &Database);
			char *ErrorMessage = 0;
			sqlite3_exec(Database, DatabaseCreationSQL, 0, 0, &ErrorMessage);
			if (ErrorMessage) {
				printf("[main]: Failed to create database! Error Message: %s\n", ErrorMessage);
				return -1; 
			}
			sqlite3_close(Database);
		}
	}

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
