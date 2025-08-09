/*
   win32service.h: win32service header file.

   Copyright 2006, 2007, 2008, 2009 Juan Rey Saura

This file is part of RELB Easy Load Balancer.

This software is a copyrighted work licensed under the Open Software License version 3.0
Please consult the file "LICENSE.txt" for details.
*/

#ifndef Win32Service_h
#define Win32Service_h

#ifdef WIN32
#include <windows.h>
#endif
/*
#ifdef UNICODE
#define TEXT(t) L##t
#define _T(t) L##t
#define T(t) L##t
#else
#define TEXT(t) t
#define _T(t) t
#define T(t) t
#endif
*/
#ifdef WIN32
#define SERVICE_NAME_T "relb"
#define SERVICE_DESCRIPTION_T "RELB Easy Load Balancer"
#endif

#ifdef WIN32
void WINAPI ServiceMain( DWORD argc, LPTSTR * argv );
void WINAPI ServiceHandler( DWORD Opcode );
bool InstallService();
bool DeleteService();
#endif

#endif
