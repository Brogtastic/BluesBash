#pragma once

#define ListenPort "8080"

#define EMAIL_LEN (100)
#define PASSWORD_LEN (45)
#define SEC_QUESTION_LEN (100)
#define SEC_ANSWER_LEN (100)
#define NICKNAME_LEN (45)

enum net_client_commands {
	Net_Client_JackIn,
	Net_Client_JackOut,
	Net_Client_Logon,
	Net_Client_SecurityQuestion,
	Net_Client_Register,
};

struct net_client_jack_in_def {
	int TransmissionLength;
};

struct net_client_jack_out_def {
	// empty
};

struct net_client_register_def {
	char Email[EMAIL_LEN];
	char Password[PASSWORD_LEN];
	char Nickname[NICKNAME_LEN];
	char SecQuestion[SEC_QUESTION_LEN];
	char SecAnswer[SEC_ANSWER_LEN];
};

struct net_client_logon_def {
	char Email[EMAIL_LEN];
	char Password[PASSWORD_LEN];
};

struct net_client_security_question_def {
	char Email[EMAIL_LEN];
	char SecurityAnswer[SEC_QUESTION_LEN];
};

// NOTE(Roskuski): calling this a nugget, because I don't think "packet" is the right word
struct net_client_nugget {
	net_client_commands Command;
	union {
		net_client_jack_in_def JackIn;
		net_client_jack_out_def JackOut;
		net_client_logon_def Logon;
		net_client_security_question_def SecurityQuestion;
		net_client_register_def Register;
		char Raw[512];
	} Data;
};

// Client ^^^
//--------------------------------
// Server vvv

enum net_server_commands {
	Net_Server_JackIn,
	Net_Server_JackOut,
	Net_Server_LogonOk,
	Net_Server_LogonFail,
	Net_Server_SecurityQuestion,
	Net_Server_SecurityQuestionOk,
	Net_Server_SecurityQuestionFail,
	Net_Server_RegisterOk,
	Net_Server_RegisterFail,
};

struct net_server_jack_in_def {
	int TransmissionLength;
};

struct net_server_jack_out_def {
	// Empty
};

struct net_server_logon_ok_def {
	char Nickname[NICKNAME_LEN];
	int UserId; // NOTE(Roskuski): This is the same id from SQL
};

struct net_server_logon_fail_def {
	// Empty
};

struct net_server_security_question_def {
	char SecQuestion[SEC_QUESTION_LEN];
};

struct net_server_security_question_ok_def {
	int PassResetAuthCode;
};

struct net_server_security_question_fail_def {
	// Empty
};

struct net_server_register_ok_def {
	int UserId;
	char Nickname[40];
};

enum net_server_register_fail_reason {
	Net_Server_RegisterFail_NoFail = 0,
	Net_Server_RegisterFail_Generic,
	Net_Server_RegisterFail_EmptyEmail,
	Net_Server_RegisterFail_EmptyPassword, 
	Net_Server_RegisterFail_EmptyNickname,
	Net_Server_RegisterFail_EmptySecQuestion,
	Net_Server_RegisterFail_EmptySecAnswer,

	Net_Server_RegisterFail_UsedEmail,
	Net_Server_RegisterFail_UsedNickname,
};

struct net_server_register_fail_def {
	net_server_register_fail_reason Reason;
};

struct net_server_nugget {
	net_server_commands Command;
	union {
		net_server_jack_in_def JackIn;
		net_server_jack_out_def JackOut;
		net_server_logon_ok_def LogonOk;
		net_server_logon_fail_def LogonFail;
		net_server_security_question_def SecQuestion;
		net_server_security_question_ok_def SecQuestionOk;
		net_server_security_question_fail_def SecQuestionFail;
		net_server_register_ok_def RegisterOk;
		net_server_register_fail_def RegisterFail;
		char Raw[512];
	} Data;
};

