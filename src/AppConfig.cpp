/* 
   AppConfig.cpp: config class source file.

   Copyright 2006, 2007, 2008, 2009 Juan Rey Saura

This file is part of Resolutive Easy Load Balancer.

This software is a copyrighted work licensed under the Open Software License version 3.0
Please consult the file "LICENSE.txt" for details.
*/

#include "AppConfig.h"
#include "utiles.h"

USING_PTYPES

#include <iostream>
#include <pstreams.h>
#include <stdlib.h>
#include <math.h>


#ifdef WIN32
#define MAX_WINDIR_PATH MAX_PATH
#endif

#define MAX_CONFIG_LINES 4000
#define DEFAULT_HTTP_ADMIN_PORT 8182
#define DEFAULT_CONNECT_TO_THE_SAME_SERVER true
#define DEFAULT_ONE_CONNECTION_PER_IP false
#define DEFAULT_CONNECTIONS_PER_THREAD 16
#define DEFAULT_UPDATE_SERVER_STATUS false
#define DEFAULT_HTTP_ADMIN_ENABLED false
#define DEFAULT_HTTP_IPADDRESS ipany
#define MINIMUN_SEVER_RECONNECTION_TIME 30

#define CONFIG_SERVER_TAG "server:"
#define CONFIG_BIND_TAG "bind:"
#define CONFIG_ALSOBIND_TAG "alsobind:"
#define CONFIG_ADMIN_TAG "admin:"
#define CONFIG_RETRY_TAG "retryserver:"
#define CONFIG_CONNECTIONS_PER_THREAD_TAG "conperthread:"
#define CONFIG_TASK_TAG "task:"  
#define CONFIG_FILTER_TAG "filter:"  

AppConfig::AppConfig()
{
   
    currentBind = 0;
    currentServet = 0;
    currentFilter = 0;
    currentAddress = 0;
    currentTask = -1;
}

bool AppConfig::getFirstBind()
{
    currentBind = 0;
    return  ( currentBind < bind.get_count() ); 
}

bool AppConfig::getNextBind()
{
    currentBind++;
    return  ( currentBind < bind.get_count() );     
}
/*
unsigned short AppConfig::getBindPort()
{
    unsigned short val = 0;

    if( currentBind < bind.get_count() )
    {
        val = bind[currentBind]->src_port;
    }

    return val;    
}

ipaddress AppConfig::getBindIP()
{
    ipaddress val = ipnone;

    if( currentBind < bind.get_count() )
    {
        val = bind[currentBind]->src_ip;
    }

    return val;    
}
*/
bool AppConfig::getFirstServer()
{
    bool val = false;
    currentServet = 0;
    
    if( currentBind < bind.get_count() )
    {        
        val = ( currentServet < bind[currentBind]->dst.get_count() );
    }
    
    return val; 
}

bool AppConfig::getNextServer()
{
    bool val = false;
        
    currentServet++;
    
    if( currentBind < bind.get_count() )
    {
        val = ( currentServet < bind[currentBind]->dst.get_count() );
    }

    return val;      
}

unsigned short AppConfig::getServerPort()
{
    unsigned short val = 0;
    
    if( currentBind < bind.get_count() )
    {
        if( currentServet < bind[currentBind]->dst.get_count() )
            val =  bind[currentBind]->dst[currentServet]->dst_port;
    }    
    
    return val;
}

ipaddress AppConfig::getServerIP()
{
    ipaddress  val = ipnone;

    if( currentBind < bind.get_count() )
    {
        if( currentServet < bind[currentBind]->dst.get_count() )
            val = bind[currentBind]->dst[currentServet]->dst_ip ;
    }

    return val;
}

string AppConfig::getServerName()
{
    string val = "void";

    if( currentBind < bind.get_count() )
    {
        if( currentServet < bind[currentBind]->dst.get_count() )
            val =  bind[currentBind]->dst[currentServet]->servername;
    }

    return val;    
}

int AppConfig::getServerMaxConnections()
{
    int val = 0;

    if( currentBind < bind.get_count() )
    {
        if( currentServet < bind[currentBind]->dst.get_count() )
            val =  bind[currentBind]->dst[currentServet]->max_connections;
    }

    return val;    
}
int AppConfig::getTasksCount()
{
    int val = 0;
    currentTask = -1;
    
    if( currentBind < bind.get_count() )
    {        
        val = bind[currentBind]->tasks.get_count();
    }
    
    return val; 
}

bool AppConfig::getNextTask()
{
    bool val = false;
        
    currentTask++;
    
    if( currentBind < bind.get_count() )
    {
        val = ( currentTask < bind[currentBind]->tasks.get_count() );
    }

    return val;      
}
/*
int AppConfig::getAddressCount()
{
    int val = 0;
    currentAddress = -1;
    
    if( currentBind < bind.get_count() )
    {        
        val = bind[currentBind]->address.get_count();
    }
    
    return val; 
}
*/
bool AppConfig::getNextAddress()
{
    bool val = false;
        
    currentAddress++;
    
    if( currentBind < bind.get_count() )
    {
        val = ( currentAddress < bind[currentBind]->address.get_count() );
    }

    return val;      
}

ipaddress AppConfig::getAddressIP()
{
    ipaddress val = ipany;

    if( currentBind < bind.get_count() )
    {
        if( currentAddress < bind[currentBind]->address.get_count() )
            val =  bind[currentBind]->address[currentAddress]->src_ip;
    }

    return val; 
}

unsigned short AppConfig::getAddressPort()
{
    unsigned short val = 0;

    if( currentBind < bind.get_count() )
    {
        if( currentAddress < bind[currentBind]->address.get_count() )
            val =  bind[currentBind]->address[currentAddress]->src_port;
    }

    return val; 
}


int AppConfig::getFilterCount()
{
    int val = 0;
    currentFilter = -1;
    
    if( currentBind < bind.get_count() )
    {        
        val = bind[currentBind]->filter.get_count();
    }
    
    return val; 
}

bool AppConfig::getNextFilter()
{
    bool val = false;
        
    currentFilter++;
    
    if( currentBind < bind.get_count() )
    {
        val = ( currentFilter < bind[currentBind]->filter.get_count() );
    }

    return val;      
}

ipaddress AppConfig::getFilterSourceIP()
{
    ipaddress val = ipany;

    if( currentBind < bind.get_count() )
    {
        if( currentFilter < bind[currentBind]->filter.get_count() )
            val =  bind[currentBind]->filter[currentFilter]->src_ip;
    }

    return val; 
}

ipaddress AppConfig::getFilterSourceMask()
{
    ipaddress val = ipany;

    if( currentBind < bind.get_count() )
    {
        if( currentFilter < bind[currentBind]->filter.get_count() )
            val =  bind[currentBind]->filter[currentFilter]->src_mask;
    }

    return val; 
}

ipaddress AppConfig::getFilterDestIP()
{
    ipaddress val = ipany;

    if( currentBind < bind.get_count() )
    {
        if( currentFilter < bind[currentBind]->filter.get_count() )
            val =  bind[currentBind]->filter[currentFilter]->dst_ip;
    }

    return val; 
}

ipaddress AppConfig::getFilterDestMask()
{
    ipaddress val = ipany;

    if( currentBind < bind.get_count() )
    {
        if( currentFilter < bind[currentBind]->filter.get_count() )
            val =  bind[currentBind]->filter[currentFilter]->dst_mask;
    }

    return val; 
}

bool AppConfig::getFilterAllow()
{
    bool val = true;

    if( currentBind < bind.get_count() )
    {
        if( currentFilter < bind[currentBind]->filter.get_count() )
            val =  bind[currentBind]->filter[currentFilter]->allow;
    }

    return val; 
}

TASK_TYPE AppConfig::getTaskType()
{
    TASK_TYPE val = INVALID_TASK;

    if( currentBind < bind.get_count() )
    {
        if( currentTask < bind[currentBind]->tasks.get_count() )
            val =  bind[currentBind]->tasks[currentTask]->task_type;
    }

    return val;      
}

int AppConfig::getTaskInterval()
{
    int val = 0;

    if( currentBind < bind.get_count() )
    {
        if( currentTask < bind[currentBind]->tasks.get_count() )
            val =  bind[currentBind]->tasks[currentTask]->run_interval_ms;
    }

    return val;       
}

bool AppConfig::getTaskFixedTime()
{
    bool val = false;

    if( currentBind < bind.get_count() )
    {
        if( currentTask < bind[currentBind]->tasks.get_count() )
            val =  bind[currentBind]->tasks[currentTask]->fixed_time;
    }

    return val;       
}

datetime AppConfig::getTaskFirstRun()
{
    datetime val = 0;

    if( currentBind < bind.get_count() )
    {
        if( currentTask < bind[currentBind]->tasks.get_count() )
            val =  bind[currentBind]->tasks[currentTask]->next_run_time;
    }

    return val;    
}

int AppConfig::getConnectionsPerThread()
{
    return connections_per_thread;
}

int AppConfig::getServerWeight()
{
    int val = 0;

    if( currentBind < bind.get_count() )
    {
        if( currentServet < bind[currentBind]->dst.get_count() )
            val =  bind[currentBind]->dst[currentServet]->weight;
    }

    return val;
}

int AppConfig::getServerRetryTime()
{
    return minimun_server_reconnection_time;
}

bool AppConfig::getAdminEnabled()
{
    return http_admin_enabled;
}

unsigned short AppConfig::getAdminPort()
{
    return http_admin_port;
}

ipaddress AppConfig::getAdminIP()
{
    return http_admin_ip;
}   
AppConfig::AppConfig( string file )
{
    configfile = file;
    AppConfig();
}

AppConfig::~AppConfig()
{
    cleanErrors();    
    cleanBindList();
}

void AppConfig::printErrors()
{
 int i = 0;
 
 while( i < errores.get_count() )
 {
    TRACE(TRACE_CONFIG)( "%s - %s\n", curr_local_time(), (char *) errores[1] );
    i++;
 }
 
}

void AppConfig::cleanErrors()
{
    while( errores.get_count() )
    {
        errores.del(0);
    }
}

bool AppConfig::loadFile()
{
    bool val = true;
    infile * file =  new infile( configfile );

    setDefaultValues();

    try
    {
        file->open();

        if( file->get_active() )
        {
            int i = 0;
            TRACE(TRACE_CONFIG)("%s - config file was opened %s\n", curr_local_time(), (const char*) configfile);
            while( !file->get_eof() && i < MAX_CONFIG_LINES )
            {
                processConfigLine( file->line() );
                i++;
            }
        }
        else
        {
            TRACE(TRACE_CONFIG)("%s - config file could not be opened %s\n", curr_local_time(), (const char*) configfile );
            val = false;
        }
    }
    catch( ... )
    {
            TRACE(TRACE_CONFIG)("%s - config file could not be open %s\n", curr_local_time(), (const char*) configfile );
            val = false;
    }

    if( file )
    {
        delete file;
        file = NULL;
    }


    if( !val )
    {
        setDefaultValues();
    }
    else
    {
        printf("Loaded config file: %s\n", (const char *)configfile );
    }

    return val;    
}

bool AppConfig::loadConfig()
{
    bool loaded = false;

    if( !isempty(configfile) )
    {
        TRACE(TRACE_CONFIG)("%s - trying the set file\n", curr_local_time());
        loaded = loadFile();
    }

    if( !loaded )
    {
        TRACE(TRACE_CONFIG)("%s - trying with the working path\n", curr_local_time());
        configfile = DEFAULT_CONFIG_FILENAME;
        loaded = loadFile();
    }

    if( !loaded )
    {
        TRACE(TRACE_CONFIG)("%s - trying the default path\n", curr_local_time());        
#ifdef WIN32
        CHAR windir[MAX_WINDIR_PATH];
        GetWindowsDirectoryA( windir, MAX_WINDIR_PATH );
        configfile = string(windir) + "\\" + DEFAULT_CONFIG_FILE_PATH + string(DEFAULT_CONFIG_FILENAME);
#else
        configfile = DEFAULT_CONFIG_FILE_PATH + string(DEFAULT_CONFIG_FILENAME);
#endif        
        loaded = loadFile();
    }

    if( loaded )
    {
        loaded = checkConfig();
        if( !loaded )
          printf("Config file was invalid\n"); 
    }

    if( !loaded )
    {
        TRACE(TRACE_CONFIG)("%s - no valid configuration\n", curr_local_time());

        printErrors();
    }

    return loaded;
}

bool AppConfig::setDefaultValues()
{
    cleanBindList();
    http_admin_port = DEFAULT_HTTP_ADMIN_PORT;
    connect_to_the_same_server = DEFAULT_CONNECT_TO_THE_SAME_SERVER;
    one_connection_per_IP = DEFAULT_ONE_CONNECTION_PER_IP;
    connections_per_thread = DEFAULT_CONNECTIONS_PER_THREAD;
    update_server_status = DEFAULT_UPDATE_SERVER_STATUS;
    http_admin_enabled = DEFAULT_HTTP_ADMIN_ENABLED;
    http_admin_ip = DEFAULT_HTTP_IPADDRESS;
    minimun_server_reconnection_time = MINIMUN_SEVER_RECONNECTION_TIME;

    return true;
}

bool AppConfig::checkConfig()
{
    bool val = true;
    int i = 0;
    int j = 0;
    
    if( http_admin_enabled )
    {
        if( !checkIP( &(http_admin_ip) ) )
        {
            TRACE(TRACE_CONFIG)("%s - admin web IP %s was not available\n", curr_local_time(), (const char *)  iptostring(http_admin_ip) );
            printf("admin web IP %s was not available\n", (const char *) iptostring(http_admin_ip) );
            val = false;
        }
        else
        {
            if( !checkPort( &(http_admin_ip), http_admin_port ) )
            {
                TRACE(TRACE_CONFIG)("%s - admin web: %d port was not available\n", curr_local_time(), http_admin_port );
                printf("admin web port %d was not available\n",  http_admin_port);
                val = false;
            }
        }
    }

    if( bind.get_count() == 0 )
    {
        val = false;
        TRACE(TRACE_CONFIG)("%s - no binding ports set\n", curr_local_time());
    }
    
    while( val && i < bind.get_count() )
    {
        if( bind[i]->address.get_count() > 0 )
        {
            for( j = 0; j < bind[i]->address.get_count() ; j++ )
            {
                if( !checkIP( &(bind[i]->address[j]->src_ip) ) )
                {
                    TRACE(TRACE_CONFIG)("%s - IP %s was not available\n", curr_local_time(), (const char *)  iptostring(bind[i]->address[j]->src_ip) );
                    printf("IP %s was not available\n", (const char *) iptostring(bind[i]->address[j]->src_ip) );
                    val = false;
                }
                else
                {
                    if( !checkPort( &(bind[i]->address[j]->src_ip), bind[i]->address[j]->src_port ) )
                    {
                        TRACE(TRACE_CONFIG)("%s - %d port was not available\n", curr_local_time(), bind[i]->address[j]->src_port );
                        printf("port %d was not available\n",  bind[i]->address[j]->src_port );
                        val = false;
                    }
                }
            }
        }
        else
        {
            val = false;
            TRACE(TRACE_CONFIG)("%s - no bind address\n", curr_local_time());
        }

        if( bind[i]->dst.get_count() == 0 )
        {
            val = false;
            TRACE(TRACE_CONFIG)("%s - no destination server\n", curr_local_time());
        }
        
        i++; 
    }

    //Check if aditional needed variables are set
    
    TRACE(TRACE_CONFIG)("%s - %s configuration\n", curr_local_time(), val?"valid":"invalid"); 

    return val;
}

void AppConfig::processConfigLine( const char * line )
{
    TRACE(TRACE_CONFIG)("%s - file line - %s\n", curr_local_time(), (const char *) line);

  if( line[0] != '/' && line[0] != ';' && line[0] != '#')
  {

    if( strncmp( (const char *) line, CONFIG_RETRY_TAG, strlen(CONFIG_RETRY_TAG)) == 0 )
    {
        TRACE(TRACE_CONFIG)("%s - retry tag\n",curr_local_time());
        int h;
        bool valid =  true;

        h = strlen(CONFIG_RETRY_TAG);
        while( line[h] == ' ' || line[h] == '\t' )
        {
            h++;
        }

        minimun_server_reconnection_time = atoi( &line[h] );
        if( minimun_server_reconnection_time == 0 )
          minimun_server_reconnection_time = MINIMUN_SEVER_RECONNECTION_TIME;        
    }

    if( strncmp( (const char *) line, CONFIG_ADMIN_TAG, strlen(CONFIG_ADMIN_TAG)) == 0 )
    {
        TRACE(TRACE_CONFIG)("%s - web admin tag\n", curr_local_time());
        int h,g;
        ipaddress dst_ip = ipany;
        unsigned short dst_port = 0;
        int weight = 0;
        int max_connections = 0;
        bool valid =  true;

        h = strlen(CONFIG_ADMIN_TAG);
        while( line[h] == ' ' || line[h] == '\t' )
        {
            h++;
        }

        g = h;
        while( line[g] != char(NULL) && line[g] != ','  && line[g] != ':')
        {  
            g++;
        }

        if( line[g] == ':' )
        {
            char  * servername =  new char[g-h+1];
            strncpy( servername, &(line[h]), g-h );
            servername[g-h] = NULL;
            dst_ip = phostbyname( servername );

            if( dst_ip == ipnone )
            {
                dst_ip = ipany;
            }
          
            TRACE(TRACE_CONFIG)("%s - web admin binding ip address %s\n", curr_local_time(), (const char *) iptostring(dst_ip) );  

            h = g +1;
        }

        dst_port = atoi( &line[h] );
        TRACE(TRACE_CONFIG)("%s - web admin bindingn port %d\n", curr_local_time(), dst_port ); 

        if( dst_port == 0 )
        {
            valid = false;
        }
     
        if( valid )
        {
            TRACE(TRACE_CONFIG)("%s - web admin valid config\n", curr_local_time());
            http_admin_ip = dst_ip;            
            http_admin_port = dst_port;
            http_admin_enabled = true;
        }
    }
    
    if( strncmp( (const char *) line, CONFIG_CONNECTIONS_PER_THREAD_TAG, strlen(CONFIG_CONNECTIONS_PER_THREAD_TAG)) == 0 )
    {
        int h = strlen(CONFIG_CONNECTIONS_PER_THREAD_TAG);
        while( line[h] == ' ' || line[h] == '\t' )
        {
            h++;
        }
        
        TRACE(TRACE_CONFIG)("%s - connections per thread tag\n", curr_local_time());
        connections_per_thread = atoi(&(line[h]));
        if( connections_per_thread == 0 )
          connections_per_thread = DEFAULT_CONNECTIONS_PER_THREAD;
    }
    
    if( strncmp( (const char *) line, CONFIG_BIND_TAG, strlen(CONFIG_BIND_TAG)) == 0 
        || strncmp( (const char *) line, CONFIG_ALSOBIND_TAG, strlen(CONFIG_ALSOBIND_TAG)) == 0 )
    {

        int h,g;
        ipaddress dst_ip = ipany;
        unsigned short dst_port = 0;
        bool valid =  true;
        bool alsobind = false ;


        alsobind = (strncmp( (const char *) line, CONFIG_ALSOBIND_TAG, strlen(CONFIG_ALSOBIND_TAG)) == 0 );
        if( alsobind )
        {
            TRACE(TRACE_CONFIG)("%s - also bind tag\n", curr_local_time());
            h = strlen(CONFIG_ALSOBIND_TAG); 
        }
        else
        {
            TRACE(TRACE_CONFIG)("%s - bind tag\n", curr_local_time());
            h = strlen(CONFIG_BIND_TAG); 
        }
        while( line[h] == ' ' || line[h] == '\t' )
        {
            h++;
        }
        
        g = h;
        while( line[g] != char(NULL) && line[g] != ','  && line[g] != ':')
        {  
            g++;
        }

        if( line[g] == ':' )
        {
            char  * servername =  new char[g-h+1];
            strncpy( servername, &(line[h]), g-h );
            servername[g-h] = NULL;
            dst_ip = phostbyname( servername );

            if( dst_ip == ipnone )
            {
                dst_ip = ipany;
            }
          TRACE(TRACE_CONFIG)("%s - bind ip address %s\n", curr_local_time(), (const char *) iptostring(dst_ip) );  

          h = g +1;
        }

        dst_port = atoi( &line[h] );
        TRACE(TRACE_CONFIG)("%s - bind destination port %d\n", curr_local_time(), dst_port );        
        if( dst_port == 0 )
        {
            valid = false;
        }
                     
        if( valid )
        {
            if( alsobind )
            {
                TRACE(TRACE_CONFIG)("%s - valid alsobind config\n", curr_local_time());
                if( bind.get_count() > 0 )
                {
                    bind[ bind.get_count() - 1 ]->address.add( new bind_address );
                    bind[ bind.get_count() - 1 ]->address[ (bind[ bind.get_count() - 1 ]->address.get_count() - 1) ]->src_ip = dst_ip;
                    bind[ bind.get_count() - 1 ]->address[ (bind[ bind.get_count() - 1 ]->address.get_count() - 1) ]->src_port = dst_port;
                }
            }
            else
            {
                TRACE(TRACE_CONFIG)("%s - valid bind config\n", curr_local_time());
                bind_conf * tmp = new bind_conf;
                bind.add( tmp );
                tmp->address.add( new bind_address );
                tmp->address[0]->src_ip = dst_ip;
                tmp->address[0]->src_port = dst_port;

            }
        }                       
    }    
    
    if( strncmp( (const char *) line, CONFIG_SERVER_TAG, strlen(CONFIG_SERVER_TAG)) == 0 )
    { 
        int h,i = 0;
        ipaddress dst_ip;
        unsigned short dst_port = 0;
        int weight = 0;
        int max_connections = 0;
        bool valid =  true;
        
        TRACE(TRACE_CONFIG)("%s - server tag\n", curr_local_time());        
        
        h = strlen(CONFIG_SERVER_TAG);

        //obviamos espacios
        while( line[h] == ' ' || line[h] == '\t' )
        {
            h++;
        }
        
        i = 0;    
        while( line[h+i] != char(NULL) && line[h+i] != ','  && line[h+i] != ':' )
            i++;
        char * servername =  new char[i+1];
 
        strncpy( servername, &(line[h]), i );
        servername[i] = NULL;
        dst_ip = phostbyname( servername );
        delete servername;
        TRACE(TRACE_CONFIG)("%s - destination server ip address %s\n", curr_local_time(), (const char *) iptostring(dst_ip) );
        if( dst_ip == ipnone )
        {
            valid = false;
        }
        
        h = h+i;
        if( line[h] ==  ',' || line[h] == ':' || line[h] == char(NULL) )
            h++;
            
        dst_port = atoi( &line[h] );
        TRACE(TRACE_CONFIG)("%s - destination server port %d\n", curr_local_time(), dst_port );
        if( dst_port == 0 )
        {
            valid = false;
        }

        if( line[h] != char(NULL) )
            h++;
            
        while( line[h] != char(NULL) && line[h] != ','  &&  line[h] != ' ' )
            h++;
 
        if( line[h] == ',' || line[h] == ' ' )
            h++;

        weight =  atoi( &line[h] );
            
        while( line[h] != char(NULL) && line[h] != ','  &&  line[h] != ' '  )
            h++;
 
        if( line[h] == ','  || line[h] == ' ' )
            h++;
            
        max_connections =  atoi( &line[h] );
        
        if( valid )
        {
            TRACE(TRACE_CONFIG)("%s - valid destination server config\n", curr_local_time());
            if( bind.get_count() > 0 )
            {
                dst_conf * tmp = new dst_conf;
                tmp->dst_ip = dst_ip;
                tmp->dst_port =  dst_port;
                tmp->weight = weight;
                tmp->max_connections = max_connections;
                
                bind[ bind.get_count() - 1 ]->dst.add( tmp );
            }
        }                         
        
    }

    if( strncmp( (const char *) line, CONFIG_TASK_TAG, strlen(CONFIG_TASK_TAG)) == 0 )
    { 
        int h,i = 0;
        int interval = 0;
        TASK_TYPE task_type = TASK_CLEAN_CONNECTIONS;
        datetime first_run = 0;
        bool fixed_time = false;
        bool valid =  true;
        
        TRACE(TRACE_CONFIG)("%s - TASK tag\n", curr_local_time());        
        
        h = strlen(CONFIG_TASK_TAG);

        //obviamos espacios
        while( line[h] == ' ' || line[h] == '\t' )
        {
            h++;
        }

        if( strncmp( &line[h], TASK_UPDATE_SERVER_INFO_TAG, strlen(TASK_UPDATE_SERVER_INFO_TAG)) == 0 )
            task_type = TASK_UPDATE_SERVER_INFO;

        if( strncmp( &line[h], TASK_CLEAN_CONNECTIONS_TAG, strlen(TASK_CLEAN_CONNECTIONS_TAG)) == 0 )
            task_type = TASK_CLEAN_CONNECTIONS;

        if( strncmp( &line[h], TASK_PURGE_CONNECTIONS_TAG, strlen(TASK_PURGE_CONNECTIONS_TAG)) == 0 )
            task_type = TASK_PURGE_CONNECTIONS;

        while( line[h] != char(NULL) && line[h] != ' ' )
            h++;

        while( line[h] == ' ' )
            h++;

        interval = atoi( &line[h] );
        TRACE(TRACE_CONFIG)("%s - task interval %d\n", curr_local_time(), interval );
        if( interval <= 0 )
        {
            valid = false;
        }


        while( line[h] != char(NULL) && line[h] != ' ' )
            h++;
        
        if( line[h] != char(NULL) )// Existe primera hora ejecución
        {
            int hour = 0;
            int min = 0;
            int sec = 0;
            int year = 1;
            int month = 1;
            int day = 1;

           while( line[h] == ' ' )
             h++;

            if( atoi( &(line[h]) ) <  24 )// si solo se especifica hora
            {
                hour = atoi( &(line[h]) );
                while( line[h] != char(NULL) && line[h] != ':' )
                  h++;
                if( line[h] != char(NULL) )
                    h++;
                min = atoi( &(line[h]) );
                while( line[h] != char(NULL) && line[h] != ':' )
                  h++;
                if( line[h] != char(NULL) )
                    h++;
                sec = atoi( &(line[h]) );
                year = 1;
                month = 1;
                day = 1;

                decodedate( NOW_UTC, year, month, day );
                first_run = encodedate( year, month, day ) + encodetime( hour, min, sec);
                if( first_run < NOW_UTC )
                    first_run = encodedate( year, month, day + 1) + encodetime( hour, min, sec);
                fixed_time = true;

            }
            else
            {
                if( atoi(&(line[h])) > 9999 )//YYYYMMDD
                {
                    year = atoi( &(line[h]) )/10000;
                    month = (atoi( &(line[h]) ) - (year * 10000))/100;
                    day = atoi( &(line[h]) )- (year * 10000) - (month * 100);

                    first_run = encodedate( year, month, day );
                    fixed_time = true;
                }
                else//YYYY/MM/DD
                {
                    year = atoi( &(line[h]) );
                    while( line[h] != char(NULL) && line[h] != '/' )
                      h++;
                    if( line[h] != char(NULL) )
                      h++;
                    month = atoi( &(line[h]) );
                    while( line[h] != char(NULL) && line[h] != '/' )
                      h++;
                    if( line[h] != char(NULL) )
                      h++;
                    day = atoi( &(line[h]) );

                    first_run = encodedate( year, month, day );
                    fixed_time = true;
                }

                //si luego hay hora

                while( line[h] != char(NULL) && line[h] != ' ' )
                    h++;

                while( line[h] == ' ' )
                    h++;

                hour = atoi( &(line[h]) );
                while( line[h] != char(NULL) && line[h] != ':' )
                  h++;
                if( line[h] != char(NULL) )
                    h++;
                min = atoi( &(line[h]) );
                while( line[h] != char(NULL) && line[h] != ':' )
                  h++;
                if( line[h] != char(NULL) )
                    h++;
                sec = atoi( &(line[h]) );

                first_run += encodetime( hour, min, sec);
            }

            valid = isvalid( first_run );
            TRACE(TRACE_CONFIG)("%s - First run (UTC) %s\n", curr_local_time(), (const char*) dttostring( first_run, "%Y%m%d %H:%M:%S"));
        }
       
        if( valid )
        {
            TRACE(TRACE_CONFIG)("%s - valid destination server config\n", curr_local_time());
            if( bind.get_count() > 0 )
            {
                task_info * tmp = new task_info;
                tmp->task_type = task_type;
                tmp->fixed_time =  fixed_time;
                tmp->run_interval_ms = interval;
                tmp->next_run_time = first_run;
                //TODO 1.1 UTC???
                
                bind[ bind.get_count() - 1 ]->tasks.add( tmp );
            }
        }                         
        
    }

    if( strncmp( (const char *) line, CONFIG_FILTER_TAG, strlen(CONFIG_FILTER_TAG)) == 0 )
    { 
        bool valid =  true;
        int h,i = 0;
        filterinfo * filter =  new filterinfo;
        filter->allow = true;
        filter->src_ip = ipany;
        filter->src_mask = ipany;
        filter->dst_ip = ipany;
        filter->dst_mask = ipany;

        TRACE(TRACE_CONFIG)("%s - filter tag\n", curr_local_time());        
        
        h = strlen(CONFIG_FILTER_TAG);
        //obviamos espacios
        while( line[h] == ' ' || line[h] == '\t' )
        {
            h++;
        }
        
        i = 0;    
        while( line[h+i] != char(NULL) && line[h+i] != '/'  && line[h+i] != ' ' )
            i++;

        //(char*)line)[h+i] = NULL;   
        char * servername =  new char[i+1];
        strncpy( servername, &(line[h]), i );
        servername[i] = NULL;
        filter->src_ip = phostbyname( servername );
        TRACE(TRACE_CONFIG)("%s - filter ip server address %s\n", curr_local_time(), servername );
        delete servername;

        TRACE(TRACE_CONFIG)("%s - filter ip source address %s\n", curr_local_time(), (const char *) iptostring(filter->src_ip) );
        if( filter->src_ip == ipnone )
        {
            valid = false;
        }
        
        h = h+i;
        i = 0;
        
        if( line[h] != char(NULL) )
            h++;

        while( line[h+i] != char(NULL) && line[h+i] != '/'  && line[h+i] != ' ' )
            i++;

        if( i > 2 )
        {
            servername =  new char[i+1];
            strncpy( servername, &(line[h]), i );
            servername[i] = NULL;
            filter->src_mask = phostbyname( servername );
            delete servername;
        }
        else
        {
            int mask_number = atoi( &(line[h]) );
            filter->src_mask = ipaddress(
                (mask_number>7)?255:(256- ( 1 << (8 - mask_number))), 
                (mask_number>15)?255:((mask_number<9)?0:(256-( 1 << (8-(mask_number-8))))),
                (mask_number>23)?255:((mask_number<17)?0:(256-( 1 << (8-(mask_number-16))))),
                (mask_number>31)?255:((mask_number<25)?0:(256-( 1<< (8-(mask_number-24))))));
            
        }

        TRACE(TRACE_CONFIG)("%s - filter ip source mask %s\n", curr_local_time(), (const char *) iptostring(filter->src_mask) );

        
        h = h+i;
        i = 0;

        if( line[h] != char(NULL) )
            h++;

        while( line[h+i] != char(NULL) && line[h+i] != '/'  && line[h+i] != ' ' )
            i++;
 
        servername =  new char[i+1];
        strncpy( servername, &(line[h]), i );
        servername[i] = NULL;
        filter->dst_ip = phostbyname( servername );
        delete servername;

        TRACE(TRACE_CONFIG)("%s - filter ip dest address %s\n", curr_local_time(), (const char *) iptostring(filter->dst_ip) );
        if( filter->dst_ip == ipnone )
        {
            valid = false;
        }
        
        h = h+i;
        i = 0;

        if( line[h] != char(NULL) )
            h++;

        while( line[h+i] != char(NULL) && line[h+i] != ' '  && line[h+i] != '\t' )
            i++;


        if( i > 2 )
        {
            servername =  new char[i+1];
            strncpy( servername, &(line[h]), i );
            servername[i] = NULL;
            filter->dst_mask = phostbyname( servername );
            delete servername;
        }
        else
        {
            int mask_number = atoi( &(line[h]) );
            filter->dst_mask = ipaddress(
                (mask_number>7)?255:(256- ( 1 << (8 - mask_number))), 
                (mask_number>15)?255:((mask_number<9)?0:(256-( 1 << (8-(mask_number-8))))),
                (mask_number>23)?255:((mask_number<17)?0:(256-( 1 << (8-(mask_number-16))))),
                (mask_number>31)?255:((mask_number<25)?0:(256-( 1<< (8-(mask_number-24))))));
            
        }

        TRACE(TRACE_CONFIG)("%s - filter ip dest mask %s\n", curr_local_time(), (const char *) iptostring(filter->dst_mask) );

        
        h = h+i;

        if( line[h] != char(NULL) )
        {
            h++;

//			while( line[h] != char(NULL) && line[h] != ' '  && line[h] != '/t' )
//				h++;

            if( strncmp( &line[h], "deny", strlen("deny")) == 0 )
            {
                filter->allow = false;
            }
        }

       
        if( valid && bind.get_count() > 0  )
        {
            TRACE(TRACE_CONFIG)("%s - valid filter\n", curr_local_time());              
            bind[ bind.get_count() - 1 ]->filter.add( filter );

        }  
        else
        {
            delete filter;
        }
    }
  } 
}

void AppConfig::cleanBindList()
{
    while( bind.get_count()  )
    {
        if( bind[0] != NULL )
        {
            while( bind[0]->dst.get_count() )
            {
                bind[0]->dst.del(0);
            }

            while( bind[0]->address.get_count() )
            {
                bind[0]->address.del(0);
            }

            while( bind[0]->filter.get_count() )
            {
                bind[0]->filter.del(0);
            }

            while( bind[0]->tasks.get_count() )
            {
                bind[0]->tasks.del(0);
            }

          delete bind[0];
        }
        bind.del(0);
    }
}
