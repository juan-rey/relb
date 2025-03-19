/* 
   main.cpp: main RELB source file.

   Copyright 2006, 2007, 2008, 2009 Juan Rey Saura

This file is part of Resolutive Easy Load Balancer.

This software is a copyrighted work licensed under the Open Software License version 3.0
Please consult the file "LICENSE.txt" for details.
*/

#include <cstdlib>
#include <iostream>
#include "Bind.h"
#include "utiles.h"
#include "AppConfig.h"
#include "relb.h"
#include "license.h"

using namespace std;

#ifdef WIN32
#include <winsock2.h>
#include "win32service.h"
#define INSTALL_SERVICE_TEXT "installservice"
#define UNINSTALL_SERVICE_TEXT "uninstallservice"
#else
#define INSTALL_SERVICE_TEXT "installdaemon"
#define UNINSTALL_SERVICE_TEXT "uninstalldaemon"
#endif

#define SHOW_LICENSE "license"
#define RUN_IN_CONSOLE_TEXT "console"
#define RUN_AS_SERVICE "service"

int main(int argc, char *argv[])
{
    bool terminar = false;
    bool console = false;
    
    printf("\n%s - %s %s (%s)\n", APP_SHORT_NAME, APP_FULL_NAME, APP_VERSION, APP_WEB);
    printf("(C) %s\n", APP_COPYRIGHT );
    printf("%s uses Hovik Melikyan's C++ Portable Types Library (PTypes) (http://www.melikyan.com/ptypes/)\n", APP_SHORT_NAME );
    printf("Usage: %s [%s] [%s] [%s] [(%s)|%s]\n\n", APP_FILE, UNINSTALL_SERVICE_TEXT, INSTALL_SERVICE_TEXT, SHOW_LICENSE, RUN_AS_SERVICE, RUN_IN_CONSOLE_TEXT);
 
#ifdef DEBUG     
console = true;
printf("DEBUG_MODE\n");
#endif
  
	if(argc>1)
	{
		
		if(strcmp(argv[1],UNINSTALL_SERVICE_TEXT)==0)
		{
#ifdef WIN32
			if(DeleteService())
				printf("\n\nService UnInstalled Sucessfully\n");
			else
				printf("\n\nError UnInstalling Service\n");
#else
				printf("\n\nNot implemented yet\n");
#endif	
			terminar = true;
		}

		if(strcmp(argv[1],INSTALL_SERVICE_TEXT)==0)
		{
#ifdef WIN32
			if(InstallService())
				printf("\n\nService Installed Sucessfully\n");
			else
				printf("\n\nError Installing Service\n");
#else
				printf("\n\nNot implemented yet\n");
#endif	
			terminar = true;
		}
	
		if(strcmp(argv[1],RUN_IN_CONSOLE_TEXT)==0)
		{
			printf("\n\nRunning in console\n");
			console = true;
		}

		if(strcmp(argv[1],SHOW_LICENSE)==0)
		{
			printf("%s",LICENSE_TEXT);
			terminar = true;
		}

	}
	
	if( !terminar )
	{
        int err;
        char salir = ' ';
        AppConfig config;
                
#ifdef WIN32
        WORD wVersionRequested;
        WSADATA wsaData;
    
        wVersionRequested = MAKEWORD(2, 0);
    
        err = WSAStartup(wVersionRequested, &wsaData);
        if ( err != 0 )
            fatal(CRIT_FIRST + 50, "WinSock initialization failure");
    
        if ( LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 0 )
            fatal(CRIT_FIRST + 51, "WinSock version mismatch (2.0 or compatible required)");
#endif
        
        if( !config.loadConfig() )
		{
			printf("I could not load config file\n");
            return 0;
		}
        config.getFirstBind();
        Bind bind_server( config.getConnectionsPerThread());
        bind_server.setAdmin( config.getAdminEnabled(), config.getAdminIP(), config.getAdminPort() );

		do
        {
            bind_server.addAddress( config.getAddressPort(), config.getAddressIP());
        }
        while( config.getNextAddress() );

        do
        {
            bind_server.addServer( (const char *) config.getServerName(),  config.getServerIP(), config.getServerPort(), config.getServerWeight(), config.getServerMaxConnections() );
        }
        while( config.getNextServer() );

		if( config.getTasksCount() > 0 )
		{
			while( config.getNextTask() )
			{
				if( config.getTaskFixedTime() )
					bind_server.addTask( config.getTaskType(), config.getTaskFirstRun(),  config.getTaskInterval() );
				else
					bind_server.addTask( config.getTaskType(), config.getTaskInterval() );
				
			}
		}

		if( config.getFilterCount() > 0 )
		{
			while( config.getNextFilter() )
			{
				bind_server.addFilter( config.getFilterSourceIP(), config.getFilterSourceMask(), config.getFilterDestIP(), config.getFilterDestMask(), config.getFilterAllow() );	
			}
		}

        bind_server.setServerRetry( config.getServerRetryTime() );
        bind_server.startListening(); 
           
        if( !console )
        {
#ifdef WIN32         
          SERVICE_TABLE_ENTRY DispatchTable[]={{ TEXT(SERVICE_NAME_T), ServiceMain},{NULL,NULL}};  
          StartServiceCtrlDispatcher(DispatchTable); 
#endif          
        }
        else
        {
            printf("Press 'x' to exit\n");
            salir = getchar();
            while( salir != 'x' )
            {
              salir = getchar();
            }          
        }
#ifdef WIN32           
        WSACleanup();  
#endif           
	}
           
    return EXIT_SUCCESS;
}
