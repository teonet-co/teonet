/**
* \file commands_creator.h
* \author max
* Created on Mon Mar 29 17:14:47 2021
*/

#ifndef COMMANDS_CREATOR_H
#define COMMANDS_CREATOR_H

#include <stdint.h>
#include <stdlib.h>

#include "ev_mgr.h"

uint8_t* createCmdConnectRPacketUdp(ksnetEvMgrClass *event_manager, size_t *size_out);

#endif