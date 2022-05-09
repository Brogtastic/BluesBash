#pragma once

#include "Server/Commands.h"

void LoadAllPppFiles();
void LoadAllUimFiles();
bool InitNetwork();
void UninitNetwork();

net_server_nugget* ClientRequest(net_client_nugget *ClientNugget);
