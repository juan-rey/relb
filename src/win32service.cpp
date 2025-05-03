/*
   win32service.cpp: win32service source file.

   Copyright 2006, 2007, 2008, 2009 Juan Rey Saura

This file is part of RELB Easy Load Balancer.

This software is a copyrighted work licensed under the Open Software License version 3.0
Please consult the file "LICENSE.txt" for details.
*/

#include "win32service.h"

#define RELB_MAXFILENAMELEN 4096

#ifdef WIN32
SERVICE_STATUS g_service_status;
SERVICE_STATUS_HANDLE g_service_status_handle;
BOOL service_running = true;

void WINAPI ServiceMain( DWORD argc, LPTSTR * argv )
{

  g_service_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
  g_service_status.dwCurrentState = SERVICE_START_PENDING;
  g_service_status.dwControlsAccepted = SERVICE_ACCEPT_STOP;
  g_service_status.dwWin32ExitCode = 0;
  g_service_status.dwServiceSpecificExitCode = 0;
  g_service_status.dwCheckPoint = 1;
  g_service_status.dwWaitHint = 0;
  g_service_status_handle = RegisterServiceCtrlHandler( TEXT( SERVICE_NAME_T ), ServiceHandler );

  if( g_service_status_handle != (SERVICE_STATUS_HANDLE) NULL )
  {
    g_service_status.dwCurrentState = SERVICE_RUNNING;
    g_service_status.dwCheckPoint = 0;
    g_service_status.dwWaitHint = 0;
    if( !SetServiceStatus( g_service_status_handle, &g_service_status ) )
    {

    }

    service_running = true;
    while( service_running )
    {
      Sleep( 1000 );
    }
  }

  return;
}


void WINAPI ServiceHandler( DWORD Opcode )
{
  switch( Opcode )
  {
    case SERVICE_CONTROL_PAUSE:
      g_service_status.dwCurrentState = SERVICE_PAUSED;
      break;

    case SERVICE_CONTROL_CONTINUE:
      g_service_status.dwCurrentState = SERVICE_RUNNING;
      break;

    case SERVICE_CONTROL_STOP:
      g_service_status.dwCurrentState = SERVICE_STOPPED;
      SetServiceStatus( g_service_status_handle, &g_service_status );
      service_running = false;
      break;

  }
}

bool DeleteService()
{
  SC_HANDLE schSCManager;
  SC_HANDLE hService;
  bool value = true;

  schSCManager = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );

  if( schSCManager )
  {
    hService = OpenService( schSCManager, TEXT( SERVICE_NAME_T ), SERVICE_ALL_ACCESS );

    if( hService )
    {
      if( DeleteService( hService ) == 0 )
        value = false;

      if( CloseServiceHandle( hService ) == 0 )
        value = false;
    }
    else
    {
      value = false;
    }
  }
  else
  {
    value = false;
  }

  return value;
}

bool InstallService()
{
  bool value = true;
#ifdef UNICODE
  WCHAR module_fileName[( RELB_MAXFILENAMELEN + 1 ) * 2];
#else
  CHAR module_fileName[RELB_MAXFILENAMELEN + 1];
#endif
  SC_HANDLE sch_scmanager, sch_service;

  GetModuleFileName( GetModuleHandle( NULL ), module_fileName, RELB_MAXFILENAMELEN );
  sch_scmanager = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );

  if( sch_scmanager )
  {

    sch_service = CreateService( sch_scmanager, TEXT( SERVICE_NAME_T ), TEXT( SERVICE_DESCRIPTION_T ),
      SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS, SERVICE_AUTO_START, SERVICE_ERROR_NORMAL,
      module_fileName, NULL, NULL, NULL, NULL, NULL );

    if( sch_service )
    {
      CloseServiceHandle( sch_service );
    }
    else
    {
      value = false;
    }
  }
  else
  {
    value = false;
  }

  return value;
}
#endif