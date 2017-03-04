/*------------------------------------------------------------------------------------------------------------------
-- HEADER FILE: Microphone.h
--
-- DATE:        March 12, 2017
--
-- DESIGNER:
--
-- PROGRAMMER:
--
-- NOTES:
-- This header file includes the required macro definitions and function declarations for microphone chat 
-- implementation (Microphone.cpp).
--------------------------------------------------------------------------------------------------------------------*/
#ifndef MICROPHONE_H
#define MICROPHONE_H

#include "common.h"
#include "AsyncSocket.h"
#include "libzplay.h"

// microphone socket struct
typedef struct 
{
	SOCKET micsocket;
	SOCKADDR_IN micaddr;

} MICSOCKET, *LPMICSOCKET;

// function prototypes
bool startMicChat();
int __stdcall micCallbackFunc(void * instance, void * user_data, libZPlay::TCallbackMessage message, 
                              unsigned int param1, unsigned int param2);

#endif