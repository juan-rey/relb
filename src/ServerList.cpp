/*
   ServerList.cpp: server list source file.

   Copyright 2006, 2007, 2008, 2009, 2025 Juan Rey Saura

This file is part of RELB Easy Load Balancer.

This software is a copyrighted work licensed under the Open Software License version 3.0
Please consult the file "LICENSE.txt" for details.
*/

#include "ServerList.h"
#include "utiles.h"
#include "htmldefs.h"

#include <stdio.h>

USING_PTYPES

#define TIMEDSEM_PROPER_PRECISION_MS 1000
#define MINIMUN_UPDATE_INTERVAL_MS  3000
#define MAXIMUN_UPDATE_INTERVAL_MS  (MINIMUN_UPDATE_INTERVAL_MS * 5)
#define TIME_SLICE_MARGIN_MS	16
#define MAX_TIME_SERVER_UNVAILABLE_MS (30 * 1000)

ServerList::ServerList(): thread( false ), status( 0 )
{
  next_task_run_time = 0;
  finish = false;
  update = false;
  parallelList = NULL;
  reconnectlostsessions = true;
  last_server_assigned = 9999;
  server_retry_mseconds = MAX_TIME_SERVER_UNVAILABLE_MS;
  jq.setTrigger( &status );
  finished_weight = 1;
  disconnected_weight = 1;
  last_connections_cleanup = NOW_UTC;
  //  start();
}

ServerList::~ServerList()
{
  update = false;
  finish = true;
  status.post();
  waitfor();
  cleanList();
  cleanPeerList();
  cleanTaskList();

}

void ServerList::setServerRetry( int seconds )
{
  server_retry_mseconds = seconds * 1000;
}

void ServerList::setPeersPerThread( int ppt )
{
  if( ppt > 0 )
    cmlist.setPeersPerThread( ppt );
}

const MessaggeQueue * ServerList::getQueue()
{
  return &jq;
}

int ServerList::cleanConnections()
{
  const peer_info * pinfo = NULL;
  int peer_index = 0;

  TRACE( TRACE_TASKS )( "%s - i am cleaning connections\n", curr_local_time() );

  peer_lock.wrlock();
  while( peer_index < peer_list.get_count() )
  {
    pinfo = peer_list[peer_index];

    if( peer_list[peer_index]->status & STATUS_PEER_NOT_CONNECTED )
    {
      int server_index = 0;

      servers_lock.rdlock();
      while( server_index < servers_list.get_count() )
      {
        if( ( servers_list[server_index]->ip == peer_list[peer_index]->dst_ip ) )
        {
          if( ( peer_list[peer_index]->status & STATUS_PEER_DISCONNECTED ) && ( servers_list[server_index]->disconnected ) )
            servers_list[server_index]->disconnected = MAX( servers_list[server_index]->disconnected - 1, 0 );
          else
            servers_list[server_index]->finished = MAX( servers_list[server_index]->finished - 1, 0 );

          server_index = servers_list.get_count();
        }
        server_index++;
      }
      servers_lock.unlock();

      if( parallelList && pinfo != NULL )
        if( pinfo->parallel )
          parallelList->post( STATUS_PEER_CONNECTION_DELETED, pintptr( pinfo->parallel ) );

      if( pinfo != NULL )
      {
        delete pinfo;
      }

      peer_list.del( peer_index );
      peer_index--;
    }

    peer_index++;
    pinfo = NULL;
  }
  peer_lock.unlock();

  return 0;
}

int ServerList::purgeConnections()
{
  peer_info * pinfo = NULL;
  TRACE( TRACE_TASKS && TRACE_VERBOSE )( "%s - i am purging connections\n", curr_local_time() );

  peer_lock.wrlock();
  int i = 0;
  while( i < peer_list.get_count() )
  {
    pinfo = peer_list[i];

    if( pinfo->manager != NULL )
      ( (ThreadedConnectionManager *) pinfo->manager )->closePInfo( pinfo );

    i++;
  }
  last_connections_cleanup = NOW_UTC;
  peer_lock.unlock();

  return -1;
}

void ServerList::cleanList()
{
  const serverinfo * pinfo = NULL;

  servers_lock.wrlock();
  while( servers_list.get_count() )
  {
    pinfo = servers_list[0];

    if( pinfo != NULL )
    {
      if( pinfo->nombre )
        delete pinfo->nombre;
      delete pinfo;
    }
    servers_list.del( 0 );
    pinfo = NULL;
  }
  servers_lock.unlock();
}

void ServerList::cleanPeerList()
{
  const peer_info * pinfo = NULL;

  peer_lock.wrlock();
  while( peer_list.get_count() )
  {
    pinfo = peer_list[0];

    if( pinfo != NULL )
    {
      delete pinfo;
    }

    peer_list.del( 0 );
    pinfo = NULL;
  }
  peer_lock.unlock();
}


void ServerList::cleanTaskList()
{
  const task_info * ptask = NULL;

  while( tasks_list.get_count() )
  {
    ptask = tasks_list[0];

    if( ptask != NULL )
    {
      delete ptask;
    }

    tasks_list.del( 0 );
    ptask = NULL;
  }

}

int ServerList::startUpdating()
{
  update = true;
  status.post();

  return 0;
}

int ServerList::stopUpdating()
{
  TRACE( TRACE_UNCATEGORIZED && TRACE_VERBOSE )( "%s - entering stopUpdating in ServerList\n", curr_local_time() );
  update = false;

  return 0;
}

bool ServerList::addFilter( const ipaddress source_ip, const ipaddress source_mask, const ipaddress dest_ip, const ipaddress dest_mask, bool allow )
{
  bool val = false;

  if( !get_running() )
  {
    filterinfo * pfilter = new filterinfo;
    pfilter->allow = allow;
    pfilter->src_ip = source_ip;
    pfilter->src_mask = source_mask;
    pfilter->dst_ip = dest_ip;
    pfilter->dst_mask = dest_mask;
    TRACE( TRACE_FILTERS )( "%s - creating %s filter for %s/%s -> %s/%s \n", curr_local_time(), allow ? "allow" : "deny", (const char *) iptostring( source_ip ), (const char *) iptostring( source_mask ), (const char *) iptostring( dest_ip ), (const char *) iptostring( dest_mask ) );
    filter_list.add( pfilter );
  }

  return val;
}


bool ServerList::addTask( TASK_TYPE task_type, int run_interval_seconds )
{
  bool val = false;

  if( !get_running() )
  {
    task_info * ptask = new task_info;
    ptask->task_type = task_type;
    ptask->run_interval_seconds = run_interval_seconds;
    ptask->next_run_time = NOW_UTC + ( 1000 * ptask->run_interval_seconds );
    if( next_task_run_time > ptask->next_run_time || !next_task_run_time )
      next_task_run_time = ptask->next_run_time;
    ptask->last_ran_time = 0;
    ptask->last_ran_exitcode = 0;
    ptask->fixed_time = false;
    TRACE( TRACE_TASKS )( "%s - new task at %s \n", curr_local_time(), given_local_time( ptask->next_run_time ) );
    tasks_list.add( ptask );
  }

  return val;
}

bool ServerList::addTask( TASK_TYPE task_type, datetime firstrun, int run_interval_seconds )
{
  bool val = false;

  if( !get_running() )
  {
    task_info * ptask = new task_info;
    ptask->task_type = task_type;
    ptask->run_interval_seconds = run_interval_seconds;
    ptask->next_run_time = firstrun;
    if( next_task_run_time > ptask->next_run_time || !next_task_run_time )
      next_task_run_time = ptask->next_run_time;
    ptask->last_ran_time = 0;
    ptask->last_ran_exitcode = 0;
    ptask->fixed_time = true;
    TRACE( TRACE_TASKS )( "%s - new task at %s \n", curr_local_time(), given_local_time( ptask->next_run_time ) );
    tasks_list.add( ptask );

    //	  if( !update && tasks_list.get_count())
    //		update = true;
  }



  return val;
}


bool ServerList::addServer( const char * nombre, const char * hostname, unsigned short puerto, int weight, int max_connections )
{
  ipaddress ip = phostbyname( hostname );

  return addServer( nombre, &ip, puerto, weight );
}

bool ServerList::addServer( const char * nombre, const ipaddress * ip, unsigned short puerto, int weight, int max_connections )
{
  serverinfo * info = new serverinfo;

  if( nombre )
  {
    info->nombre = new char[strlen( nombre ) + 1];
    strcpy( info->nombre, nombre );
  }
  else
  {
    info->nombre = new char[1];
    info->nombre[0] = 0;
  }

  info->ip = *ip;
  info->port = puerto;
  info->disconnected = 0;
  info->finished = 0;
  info->connected = 0;
  info->connecting = 0;
  info->weight = weight;
  info->max_connections = max_connections;
  info->last_connection_failed = false;
  info->last_connection_attempt = NOW_UTC;
  servers_lock.wrlock();
  servers_list.add( info );
  servers_lock.unlock();

  return true;
}

/// <summary>
/// Process the messages in the queue and update the server list. 
/// Also, it executes the tasks in the task list.
/// </summary>
/// <remarks> it runs in a separate thread. 
/// messages are dispatched from execute in AdminHTTPServver and ThreadedConnectionManager (using ConnectionPeer) to this class.
/// </remarks>
void ServerList::execute()
{
  message * msg = NULL;
  peer_info * pinfo = NULL;
  //int i = 0;
  int current_item_index = 0;
  int wait_ms = MINIMUN_UPDATE_INTERVAL_MS;


  if( !update && tasks_list.get_count() )
    update = true;


  while( !finish )
  {
    //TRACE( TRACE_UNCATEGORIZED && TRACE_VERBOSE )( "%s - checking message in ServerList\n", curr_local_time() );
    msg = jq.getmessage( 0 );
    //TRACE( TRACE_UNCATEGORIZED && TRACE_VERBOSE )( "%s - found message %p in ServerList\n", curr_local_time(), msg );

    while( msg != NULL && !finish )
    {
      TRACE( TRACE_UNCATEGORIZED && TRACE_VERBOSE )( "%s - processing message in ServerList\n", curr_local_time() );
      current_item_index = 0;
      pinfo = NULL;

      peer_lock.rdlock();
      while( !pinfo && current_item_index < peer_list.get_count() )
      {
        if( peer_list[current_item_index] == (peer_info *) msg->param )
        {
          // if it wasn't disconnected? <- not here?
          pinfo = peer_list[current_item_index];
          //current_item_index++;
        }
        else
        {
          current_item_index++;
        }
      }

      if( pinfo != NULL )
      {
        switch( msg->id )
        {
          case STATUS_PEER_CONNECTION_CANCELLED:
            break;
          case STATUS_PEER_CONNECTION_KICKED:
            TRACE( TRACE_CONNECTIONS )( "%s - kick request\n", curr_local_time() );
            if( pinfo->manager != NULL )
              ( (ThreadedConnectionManager *) pinfo->manager )->closePInfo( pinfo );
            break;
          default:
          {
            pinfo->status = msg->id;
            pinfo->modified = NOW_UTC;
            if( parallelList && pinfo->parallel )
              parallelList->post( pinfo->status, pintptr( pinfo->parallel ) );
          }
        }

        switch( msg->id & STATUS_CLIENT_SERVER_MASK )
        {
          // Here the message is forwarded and more processing is performed
          case STATUS_CONNECTION_LOST:
          case STATUS_DISCONNECTED_OK:
          case STATUS_CONNECTION_FAILED:
          {
            int i = 0;
            int peer_index = 0;
            bool hay_mas_conectados = false;
            while( peer_index < peer_list.get_count() && !hay_mas_conectados )
            {
              if( ( peer_index != current_item_index ) && !( peer_list[peer_index]->status & STATUS_PEER_NOT_CONNECTED ) && ( peer_list[peer_index]->src_ip == pinfo->src_ip ) )
              {
                hay_mas_conectados = true;
              }
              else
              {
                peer_index++;
              }
            }

            servers_lock.rdlock();
            while( i < servers_list.get_count() )
            {
              if( ( servers_list[i]->ip == pinfo->dst_ip ) && ( servers_list[i]->port == pinfo->dst_port ) )
              {
                if( msg->id == STATUS_CONNECTION_FAILED )
                {
                  servers_list[i]->last_connection_failed = true;
                  servers_list[i]->last_connection_attempt = NOW_UTC;
                }

                if( msg->id & STATUS_CONNECTION_FAILED )
                  servers_list[i]->connecting = MAX( servers_list[i]->connecting - 1, 0 );
                else
                  servers_list[i]->connected = MAX( servers_list[i]->connected - 1, 0 );

                if( ( hay_mas_conectados || ( pinfo->created < last_connections_cleanup ) ) && current_item_index > -1 && current_item_index < peer_list.get_count() )
                {
                  pinfo = NULL;
                  peer_list.del( current_item_index );
                }
                else
                {
                  if( msg->id & STATUS_PEER_DISCONNECTED )
                    servers_list[i]->disconnected++;
                  else
                    servers_list[i]->finished++;
                }

                TRACE( TRACE_CONNECTIONS )( "%s - ServerList - server %d, connecting %d, connected %d, finished %d, disconnected %d, total %d connections\n", curr_local_time(), i, servers_list[i]->connecting, servers_list[i]->connected, servers_list[i]->finished, servers_list[i]->disconnected, servers_list[i]->connecting + servers_list[i]->connected + servers_list[i]->finished + servers_list[i]->disconnected );
                i = servers_list.get_count();
              }
              i++;
            }
            servers_lock.unlock();
          }
          break;
          case STATUS_CONNECTION_ESTABLISHED:
          {
            int i = 0;
            servers_lock.rdlock();
            while( i < servers_list.get_count() )
            {
              if( ( servers_list[i]->ip == pinfo->dst_ip ) && ( servers_list[i]->port == pinfo->dst_port ) )
              {
                servers_list[i]->connected++;
                servers_list[i]->connecting = MAX( servers_list[i]->connecting - 1, 0 );
                servers_list[i]->last_connection_failed = false;
                servers_list[i]->last_connection_attempt = NOW_UTC;
                TRACE( TRACE_CONNECTIONS )( "%s - ServerList - server %d, connecting %d, connected %d, finished %d, disconnected %d, total %d connections\n", curr_local_time(), i, servers_list[i]->connecting, servers_list[i]->connected, servers_list[i]->finished, servers_list[i]->disconnected, servers_list[i]->connecting + servers_list[i]->connected + servers_list[i]->finished + servers_list[i]->disconnected );
                i = servers_list.get_count();
              }
              i++;
            }
            servers_lock.unlock();
          }
          break;
        }

      }
      peer_lock.unlock();

      delete msg;
      msg = NULL;
      msg = jq.getmessage( 0 );//TODO ???
    }

    if( update )
    {
      TRACE( TRACE_TASKS )( "%s - preprocessing tasks %s \n", curr_local_time(), given_local_time( next_task_run_time ) );
      if( next_task_run_time && ( NOW_UTC >= next_task_run_time ) )
      {
        TRACE( TRACE_TASKS )( "%s - processing tasks %s \n", curr_local_time(), given_local_time( next_task_run_time ) );
        task_info * ptask = NULL;
        int task_index = 0;
        bool delete_task;

        while( task_index < tasks_list.get_count() )
        {
          TRACE( TRACE_TASKS && TRACE_VERY_VERBOSE )( "%s - checking tasks - index %d \n", curr_local_time(), task_index );
          ptask = tasks_list[task_index];

          if( ptask != NULL )
          {
            delete_task = false;

            if( NOW_UTC >= ptask->next_run_time )
            {
              TRACE( TRACE_TASKS )( "%s - found a task to run\n", curr_local_time() );
              ptask->last_ran_time = NOW_UTC;

              //TODO TASK_UPDATE_SERVER_INFO
              //if( ptask->task_type == TASK_UPDATE_SERVER_INFO )
              //  ptask->last_ran_exitcode = updateServerInfo();

              if( ptask->task_type == TASK_CLEAN_CONNECTIONS )
                ptask->last_ran_exitcode = cleanConnections();

              if( ptask->task_type == TASK_PURGE_CONNECTIONS )
                ptask->last_ran_exitcode = purgeConnections();

              if( ptask->run_interval_seconds > 0 )
              {
                if( ptask->fixed_time )
                {
                  if( NOW_UTC >= ptask->next_run_time )
                  {
                    ptask->next_run_time = ptask->next_run_time + ( ( ( ( NOW_UTC - ptask->next_run_time ) / ( 1000 * ptask->run_interval_seconds ) ) + 1 ) * ( 1000 * ptask->run_interval_seconds ) );
                    TRACE( TRACE_TASKS )( "%s - next run %s\n", curr_local_time(), given_local_time( ptask->next_run_time ) );
                  }
                }
                else
                {
                  ptask->next_run_time = NOW_UTC + ( 1000 * ptask->run_interval_seconds );
                  TRACE( TRACE_TASKS )( "%s - the next run %s\n", curr_local_time(), given_local_time( ptask->next_run_time ) );
                }
              }
              else
              {
                TRACE( TRACE_TASKS )( "%s - Single time execution task, deleting\n", curr_local_time() );
                delete_task = true;
              }
            }

            if( ( ptask->next_run_time < next_task_run_time ) || ( NOW_UTC >= next_task_run_time ) )
            {
              next_task_run_time = ptask->next_run_time;
            }

            if( delete_task )
            {
              delete ptask;
              tasks_list.del( task_index );
              task_index--;
              if( tasks_list.get_count() == 0 )
              {
                next_task_run_time = NOW_UTC + MINIMUN_UPDATE_INTERVAL_MS;
              }
            }
          }
          task_index++;
        }
        TRACE( TRACE_TASKS )( "%s - finished processing tasks, next on %s \n", curr_local_time(), given_local_time( next_task_run_time ) );
      }

      //wait_ms = MIN( int( next_task_run_time - NOW_UTC + TIME_SLICE_MARGIN_MS ), MAXIMUN_UPDATE_INTERVAL_MS );
      wait_ms = ( ( ( MIN( int( next_task_run_time - NOW_UTC + TIME_SLICE_MARGIN_MS ), MAXIMUN_UPDATE_INTERVAL_MS - TIMEDSEM_PROPER_PRECISION_MS ) ) / TIMEDSEM_PROPER_PRECISION_MS ) * TIMEDSEM_PROPER_PRECISION_MS ) + TIMEDSEM_PROPER_PRECISION_MS;
      TRACE( TRACE_TASKS )( "%s - waiting next tasks %d \n", curr_local_time(), wait_ms );

    }

    //TRACE( TRACE_UNCATEGORIZED && TRACE_VERY_VERBOSE )( "%s - entering wait for %d ms\n", curr_local_time(), wait_ms );
    // IMPORTANT - WORKAROUND
    // Found a performance issue in Ptypes timedsem.wait() on non Win32 platforms 
    // The wait time should be a multiple of a whole second to avoid any performance penalty 
    status.wait( wait_ms );
    //TRACE( TRACE_UNCATEGORIZED && TRACE_VERY_VERBOSE )( "%s - exiting wait for %d ms\n", curr_local_time(), wait_ms );
  }
}

void ServerList::cleanup()
{
}

/// <summary>
/// Determines the server to be used for the connection. 
/// It checks if there is a previous connection to the same server and if it is still valid. 
/// If not, it assigns the most available server from the list of servers.
/// </summary>
/// 
/// <returns> pointer to the peer_info structure that contains the information of the server to be used and the client, only when a server is available </returns>
const peer_info * ServerList::getServer( const ipaddress * ipcliente, unsigned short puertocliente, ThreadedConnectionManager * pcm )
{

  peer_info * pinfo = NULL;
  peer_info * pinfoparallel = NULL;

  servers_lock.rdlock();
  // Only if there are servers  
  if( servers_list.get_count() )
  {
   

    if( reconnectlostsessions )
    {
      int peer_index = 0;
      datetime lostsessionmodified = NOW_UTC;
      int lostsessionindex = -1;
      int lostsessionserver = -1;
      int server_index = -1;
      TRACE( TRACE_ASSIGNATION )( "%s - checking desconnections\n", curr_local_time() );

      peer_lock.rdlock();
      // Track previous ones while no exact match is found
      while( !pinfo && peer_index < peer_list.get_count() )
      {
        server_index = -1;
        // If there is a possible one
        if( peer_list[peer_index]->src_ip == *ipcliente )
        {
          // If the connection is valid              
          if( peer_list[peer_index]->status != STATUS_CONNECTION_FAILED )
          {
            server_index = servers_list.get_count(); // if always > 0 we have already checked -> if( servers_list.get_count() )
            server_index--;

            while( server_index > 0 && servers_list[server_index]->ip != peer_list[peer_index]->dst_ip )
              server_index--;

            if( server_index == 0 && servers_list[server_index]->ip != peer_list[peer_index]->dst_ip )
              server_index--;

            // There was a matching server
            if( server_index >= 0 )
            {
              // Discard if the reconnection time after a failure has not passed
              if( servers_list[server_index]->last_connection_failed && ( ( NOW_UTC - servers_list[server_index]->last_connection_attempt ) < server_retry_mseconds ) )
              {
                server_index = -1;
              }
            }
          }

          // There was a previous valid session                
          if( server_index >= 0 )
          {
            if( peer_list[peer_index]->src_port == puertocliente )
            {
              // Just in case, disconnect
              if( peer_list[peer_index]->manager != NULL )
                ( (ThreadedConnectionManager *) peer_list[peer_index]->manager )->closePInfo( pinfo );

              servers_list[server_index]->connecting++;
              // DOUBT: Should I decrease disconnected?

              pinfo = peer_list[peer_index];
              pinfo->src_port = puertocliente;
              pinfo->status = STATUS_CONNECTING_SERVER;
              pinfo->modified = NOW_UTC;
              pinfo->manager = (void *) pcm;

              if( parallelList && pinfo->parallel )
                parallelList->post( pinfo->status, pintptr( pinfo->parallel ) );
            }
            else
            {
              // Found but there was already a preassigned one
              if( lostsessionindex >= 0 )
              {
                TRACE( TRACE_ASSIGNATION )( "%s - found another disconnection %d to server with ip address %s\n", curr_local_time(), peer_index, (const char *) iptostring( peer_list[peer_index]->dst_ip ) );

                // The connection is after the one that was preassigned
                if( ( peer_list[peer_index]->modified > peer_list[lostsessionindex]->modified ) && ( peer_list[lostsessionindex]->status != STATUS_CONNECTION_FAILED ) )
                {
                  lostsessionindex = peer_index;
                }
                else
                {
                  // The previously preassigned connection failed
                  if( peer_list[lostsessionindex]->status == STATUS_CONNECTION_FAILED )
                  {
                    lostsessionindex = peer_index;
                  }
                }
              }
              else
              {
                TRACE( TRACE_ASSIGNATION )( "%s - found a disconnection %d to server with ip %s\n", curr_local_time(), peer_index, (const char *) iptostring( peer_list[peer_index]->dst_ip ) );

                // If there was no preassigned one, accept this one
                lostsessionindex = peer_index;
              }

              // If the peer is valid, so is the server
              if( lostsessionindex == peer_index )
              {
                lostsessionserver = server_index;
              }
            }
          }
        }
        peer_index++;
      }
      peer_lock.unlock();

      // If something was found and needs to be recreated
      if( lostsessionindex >= 0 && !pinfo )
      {
        peer_lock.wrlock();
        if( lostsessionindex >= 0 && lostsessionindex < peer_list.get_count() && !pinfo )
        {

          //if( servers_list[lostsessionserver]->ip == peer_list[lostsessionindex]->dst_ip )
          if( peer_list[lostsessionindex]->src_ip == *ipcliente )
          {
            if( lostsessionserver >= 0 && lostsessionserver < servers_list.get_count() )
            {
              servers_list[lostsessionserver]->connecting++;
              TRACE( TRACE_CONNECTIONS )( "%s - ServerList - server %d, connecting %d, connected %d, finished %d, disconnected %d, total %d connections\n", curr_local_time(), lostsessionserver, servers_list[lostsessionserver]->connecting, servers_list[lostsessionserver]->connected, servers_list[lostsessionserver]->finished, servers_list[lostsessionserver]->disconnected, servers_list[lostsessionserver]->connecting + servers_list[lostsessionserver]->connected + servers_list[lostsessionserver]->finished + servers_list[lostsessionserver]->disconnected );
            }

            pinfo = new peer_info;
            pinfo->src_ip = *ipcliente;
            pinfo->src_port = puertocliente;
            pinfo->dst_ip = peer_list[lostsessionindex]->dst_ip;
            pinfo->dst_port = peer_list[lostsessionindex]->dst_port;
            pinfo->status = STATUS_CONNECTING_SERVER;
            pinfo->created = NOW_UTC;
            pinfo->modified = pinfo->created;
            pinfo->manager = NULL;
            pinfo->ban_all = false;
            pinfo->ban_this = false;
            pinfo->parallel = NULL;
            pinfo->manager = (void *) pcm;

            // Remove disconnected connections from the list
            peer_index = 0;
            while( peer_index < peer_list.get_count() )
            {
              if( ( peer_list[peer_index]->src_ip == *ipcliente ) && ( peer_list[peer_index]->status & STATUS_PEER_NOT_CONNECTED ) )
              {
                TRACE( TRACE_ASSIGNATION )( "%s - Deleting lost connection %d\n", curr_local_time(), peer_index );

                if( lostsessionserver >= 0 && lostsessionserver < servers_list.get_count() )
                {
                  if( peer_list[peer_index]->status & STATUS_PEER_DISCONNECTED )
                    servers_list[lostsessionserver]->disconnected = MAX( servers_list[lostsessionserver]->disconnected - 1, 0 );
                  else
                    servers_list[lostsessionserver]->finished = MAX( servers_list[lostsessionserver]->finished - 1, 0 );
                }

                peer_list.del( peer_index );
              }
              else
              {
                peer_index++;
              }
            }


            peer_list.add( pinfo );

            if( parallelList )
            {
              pinfoparallel = new peer_info;
              pinfo->setParallel( pinfoparallel );

              if( parallelList && pinfoparallel )
                parallelList->post( pinfoparallel->status, pintptr( pinfoparallel ) );
            }
          }


        }
        peer_lock.unlock();
      }
    }


    // If not preassigned by reconnection, assign a server                     
    if( !pinfo )
    {
      int nextserver, currentserver;
      int nextserver_weight, currentserver_weight;
      float nextserver_index, currentserver_index;
      bool nextserver_notavailable, currentserver_notavailable;
      bool assigned_server = false;

      // Assign (nextserver) the next to the last assigned, calculate ratio and availability, and move to the next one    
      last_server_assigned++;

      if( last_server_assigned >= servers_list.get_count() )
      {
        last_server_assigned = 0;
      }

      TRACE( TRACE_ASSIGNATION )( "%s - Evaluating servers from %d\n", curr_local_time(), last_server_assigned );
      currentserver = last_server_assigned;
      currentserver_index = ( servers_list[currentserver]->connecting + servers_list[currentserver]->connected + ( servers_list[currentserver]->finished * finished_weight ) + ( servers_list[currentserver]->disconnected * disconnected_weight ) ) / float( MAX( 1, servers_list[currentserver]->weight ) );
      currentserver_notavailable = ( ( servers_list[currentserver]->max_connections && servers_list[currentserver]->connected >= servers_list[currentserver]->max_connections )
        || ( servers_list[currentserver]->last_connection_failed && ( ( NOW_UTC - servers_list[currentserver]->last_connection_attempt ) < server_retry_mseconds ) ) );
      nextserver_weight = servers_list[currentserver]->weight;
      nextserver_index = currentserver_index;
      nextserver_notavailable = currentserver_notavailable;
      nextserver = currentserver;
      TRACE( TRACE_ASSIGNATION )( "%s - Setting default server %d with index %f  weight %d and %d connections, last connection was %s %d seconds ago\n",
        curr_local_time(), currentserver, currentserver_index, servers_list[currentserver]->weight, servers_list[currentserver]->connecting + servers_list[currentserver]->connected + servers_list[currentserver]->finished + servers_list[currentserver]->disconnected, currentserver_notavailable ? "failed" : "succeeded", int( NOW_UTC - servers_list[currentserver]->last_connection_attempt ) / 1000 );

      if( isServerAllowed( &( servers_list[nextserver]->ip ), ipcliente ) )
        assigned_server = true;

      currentserver++;

      if( currentserver >= servers_list.get_count() )
      {
        currentserver = 0;
      }


      // Review why if last_server_assigned is 0 it is checked twice
      // I check all and assign (nextserver) the best one
      while( currentserver != last_server_assigned )
      {
        // Calculate availability ratio for this server
        currentserver_index = ( servers_list[currentserver]->finished + servers_list[currentserver]->connected + ( servers_list[currentserver]->finished * finished_weight ) + ( servers_list[currentserver]->disconnected * disconnected_weight ) ) / float( MAX( 1, servers_list[currentserver]->weight ) );
        currentserver_notavailable = ( ( servers_list[currentserver]->max_connections && servers_list[currentserver]->connected >= servers_list[currentserver]->max_connections )
          || ( servers_list[currentserver]->last_connection_failed && ( ( NOW_UTC - servers_list[currentserver]->last_connection_attempt ) < server_retry_mseconds ) ) );
        currentserver_weight = servers_list[currentserver]->weight;
        TRACE( TRACE_ASSIGNATION )( "%s - Analysing server %d with index %f weight %d and %d connections, last connection %s %d seconds ago\n",
          curr_local_time(), currentserver, currentserver_index, servers_list[currentserver]->weight, ( servers_list[currentserver]->finished + servers_list[currentserver]->connected + servers_list[currentserver]->disconnected + servers_list[currentserver]->connecting ), currentserver_notavailable ? "failed" : "succeeded", int( NOW_UTC - servers_list[currentserver]->last_connection_attempt ) / 1000 );

        // Evaluate IP filters
        if( isServerAllowed( &( servers_list[currentserver]->ip ), ipcliente ) )
        {

          TRACE( TRACE_ASSIGNATION )( "%s - Allowed server %d with index %f weight %d, next index %f weight %d\n",
            curr_local_time(), currentserver, currentserver_index, currentserver_weight, nextserver_index, nextserver_weight );
          // here or elsewhere???
          //assigned_server = true;	
          if( !currentserver_index && !nextserver_index )
          {
            // They have no connections (index is 0)          
            if( ( !assigned_server || currentserver_weight > nextserver_weight )
              && ( !currentserver_notavailable || ( currentserver_notavailable && nextserver_notavailable ) )
              )
            {
              nextserver = currentserver;
              nextserver_weight = currentserver_weight;
              nextserver_index = currentserver_index;
              nextserver_notavailable = currentserver_notavailable;
              assigned_server = true;
            }

          }
          else
          {
            // They have connections (index is not 0)           
            if( ( !assigned_server || currentserver_index < nextserver_index )
              && ( !currentserver_notavailable || ( currentserver_notavailable && nextserver_notavailable ) )
              )
            {
              nextserver = currentserver;
              nextserver_weight = currentserver_weight;
              nextserver_index = currentserver_index;
              nextserver_notavailable = currentserver_notavailable;
              assigned_server = true;
            }
          }

        }

        currentserver++;

        if( currentserver >= servers_list.get_count() )
        {
          currentserver = 0;
        }
      }


      // If nothing was found, make one last attempt including non-assignable servers
      if( !assigned_server )
      {
        TRACE( TRACE_ASSIGNATION )( "%s - Ooops, no valid server found. Re-check even non available servers\n", curr_local_time() );
        while( currentserver != last_server_assigned )
        {
          // Calculate availability ratio for this server
          currentserver_index = ( servers_list[currentserver]->finished + servers_list[currentserver]->connected + ( servers_list[currentserver]->finished * finished_weight ) + ( servers_list[currentserver]->disconnected * disconnected_weight ) ) / float( MAX( 1, servers_list[currentserver]->weight ) );
          currentserver_notavailable = false;
          currentserver_weight = servers_list[currentserver]->weight;
          TRACE( TRACE_ASSIGNATION )( "%s - Analizing server %d with index %f weight %d y %d connections, last connection %s %d seconds ago\n",
            curr_local_time(), currentserver, currentserver_index, servers_list[currentserver]->weight, ( servers_list[currentserver]->finished + servers_list[currentserver]->connected + servers_list[currentserver]->disconnected + servers_list[currentserver]->connecting ), currentserver_notavailable ? "failed" : "succeeded", int( NOW_UTC - servers_list[currentserver]->last_connection_attempt ) / 1000 );

          // Evaluate IP filters
          if( isServerAllowed( &( servers_list[currentserver]->ip ), ipcliente ) )
          {

            TRACE( TRACE_ASSIGNATION )( "%s - Allowed server %d with index %f weight %d, next index %f weight %d\n",
              curr_local_time(), currentserver, currentserver_index, currentserver_weight, nextserver_index, nextserver_weight );
            // here or elsewhere???
            //assigned_server = true;
            if( !currentserver_index && !nextserver_index )
            {
              // They have no connections (index is 0)       
              if( ( !assigned_server || currentserver_weight > nextserver_weight )
                && ( !currentserver_notavailable || ( currentserver_notavailable && nextserver_notavailable ) )
                )
              {
                nextserver = currentserver;
                nextserver_weight = currentserver_weight;
                nextserver_index = currentserver_index;
                nextserver_notavailable = currentserver_notavailable;
                assigned_server = true;
              }

            }
            else
            {
              // They have connections (index is not 0)           
              if( ( !assigned_server || currentserver_index < nextserver_index )
                && ( !currentserver_notavailable || ( currentserver_notavailable && nextserver_notavailable ) )
                )
              {
                nextserver = currentserver;
                nextserver_weight = currentserver_weight;
                nextserver_index = currentserver_index;
                nextserver_notavailable = currentserver_notavailable;
                assigned_server = true;
              }
            }

          }

          currentserver++;

          if( currentserver >= servers_list.get_count() )
          {
            currentserver = 0;
          }
        }

      }

      if( assigned_server )
      {
        // got the server to be used
        last_server_assigned = nextserver;
        if( last_server_assigned < 0 || last_server_assigned >= servers_list.get_count() )
        {
          last_server_assigned = 0;
        }
        servers_list[last_server_assigned]->connecting++;
        TRACE( TRACE_CONNECTIONS )( "%s - ServerList - server %d, connecting %d, connected %d, finished %d, disconnected %d, total %d connections\n", curr_local_time(), last_server_assigned, servers_list[last_server_assigned]->connecting, servers_list[last_server_assigned]->connected, servers_list[last_server_assigned]->finished, servers_list[last_server_assigned]->disconnected, servers_list[last_server_assigned]->connecting + servers_list[last_server_assigned]->connected + servers_list[last_server_assigned]->finished + servers_list[last_server_assigned]->disconnected );

        pinfo = new peer_info;
        pinfo->src_ip = *ipcliente;
        pinfo->src_port = puertocliente;
        pinfo->dst_ip = servers_list[last_server_assigned]->ip;
        pinfo->dst_port = servers_list[last_server_assigned]->port;
        pinfo->status = STATUS_CONNECTING_SERVER;
        pinfo->created = NOW_UTC;
        pinfo->modified = pinfo->created;
        pinfo->ban_all = false;
        pinfo->ban_this = false;
        pinfo->parallel = NULL;
        pinfo->manager = (void *) pcm;
        TRACE( TRACE_ASSIGNATION )( "%s - Assigning server %d with IP %s\n", curr_local_time(), last_server_assigned, (const char *) iptostring( pinfo->dst_ip ) );

        if( parallelList )
        {
          pinfoparallel = new peer_info;
          pinfo->setParallel( pinfoparallel );

          if( parallelList && pinfoparallel )
            parallelList->post( pinfoparallel->status, pintptr( pinfoparallel ) );
        }

        peer_lock.wrlock();
        peer_list.add( pinfo );
        peer_lock.unlock();
      }
      else
      {
        TRACE( TRACE_ASSIGNATION )( " %s - NO SERVER FOUND\n", curr_local_time() );
      }

      TRACE( TRACE_ASSIGNATION )( " %s - finishing assignation\n", curr_local_time() );
    }
  }
  servers_lock.unlock();

  return pinfo;
}

bool ServerList::isServerAllowed( const ipaddress * server, const ipaddress * client )
{
  bool val = true;

  TRACE( TRACE_FILTERS )( "%s - Checking filter for client %s and server %s\n", curr_local_time(), (const char *) iptostring( *client ), (const char *) iptostring( *server ) );

  if( filter_list.get_count() )
  {
    int i = 0;

    while( i < filter_list.get_count() )
    {
      TRACE( TRACE_FILTERS && TRACE_VERBOSE )( "%s - examinig filter: client %s and server %s %s/%s %s/%s %s\n", curr_local_time(),
        (const char *) iptostring( *client ), (const char *) iptostring( *server ), (const char *) iptostring( filter_list[i]->src_ip ), (const char *) iptostring( filter_list[i]->src_mask ),
        (const char *) iptostring( filter_list[i]->dst_ip ), (const char *) iptostring( filter_list[i]->dst_mask ), ( filter_list[i]->allow ) ? "allow" : "deny" );
      TRACE( TRACE_FILTERS && TRACE_VERBOSE )( "%s - client %s %s and server %s %s\n",
        curr_local_time(), (const char *) iptostring( masked_ip( client, &( filter_list[i]->src_mask ) ) ), (const char *) iptostring( masked_ip( &( filter_list[i]->src_ip ), &( filter_list[i]->src_mask ) ) ),
        (const char *) iptostring( masked_ip( server, &( filter_list[i]->dst_mask ) ) ), (const char *) iptostring( masked_ip( &( filter_list[i]->dst_ip ), &( filter_list[i]->dst_mask ) ) ) );

      if( masked_ip( client, &( filter_list[i]->src_mask ) ) == masked_ip( &( filter_list[i]->src_ip ), &( filter_list[i]->src_mask ) ) )
        if( masked_ip( server, &( filter_list[i]->dst_mask ) ) == masked_ip( &( filter_list[i]->dst_ip ), &( filter_list[i]->dst_mask ) ) )
        {
          TRACE( TRACE_FILTERS )( "%s - %s filter match client %s and server %s %s/%s %s/%s\n", curr_local_time(), ( filter_list[i]->allow ) ? "allow" : "deny", (const char *) iptostring( *client ), (const char *) iptostring( *server ), (const char *) iptostring( filter_list[i]->src_ip ), (const char *) iptostring( filter_list[i]->src_mask ), (const char *) iptostring( filter_list[i]->dst_ip ), (const char *) iptostring( filter_list[i]->dst_mask ) );
          val = filter_list[i]->allow;
          i = filter_list.get_count() + 1;
        }
      i++;
#ifdef DEBUG
      if( i == filter_list.get_count() )
        TRACE( TRACE_FILTERS )( "%s - No filter found for client %s and server %s\n", curr_local_time(), (const char *) iptostring( *client ), (const char *) iptostring( *server ) );
#endif
    }
  }

  TRACE( TRACE_FILTERS )( "%s - Connection for client %s to server %s is %s\n", curr_local_time(), (const char *) iptostring( *client ), (const char *) iptostring( *server ), ( val ) ? "allowed" : "denied" );
  return val;
}


void ServerList::setServer( int client_socket, sockaddr_in * sac )
{
  ipaddress temp( sac->sin_addr.s_addr );
  ThreadedConnectionManager * pcm = cmlist.getFreeThreadedConnectionManager();
  Socket * pCliente = new Socket( client_socket, &temp, ntohs( sac->sin_port ) );
  Socket * pServidor = NULL;
  peer_info * pinfo = NULL;

  if( pinfo = (peer_info *) getServer( (ipaddress *) &( sac->sin_addr.s_addr ), ntohs( sac->sin_port ), pcm ) )
  {
    TRACE( TRACE_CONNECTIONS )( "%s - Assigned server with ip %s\n", curr_local_time(), (const char *) iptostring( pinfo->dst_ip ) );
    pServidor = new Socket( &( pinfo->dst_ip ), pinfo->dst_port );
    ConnectionPeer * cpeer = new ConnectionPeer( pCliente, pServidor, getQueue(), pinfo );
    pcm->addConectionPeer( cpeer );
  }
  else
  {
    TRACE( TRACE_CONNECTIONS )( "%s - No server was assigned for the client\n", curr_local_time() );
    if( pCliente )
      delete pCliente;
    pCliente = NULL;
  }
}

void ServerList::setPararllelList( const MessaggeQueue * jq )
{
  const peer_info * pinfo = NULL;
  peer_info * pinfoparallel = NULL;
  peer_lock.rdlock();
  int i = peer_list.get_count();
  parallelList = (MessaggeQueue *) jq;

  if( parallelList )
  {
    while( i )
    {
      i--;
      pinfo = peer_list[i];

      if( pinfo != NULL )
      {

        if( pinfo->parallel )
        {
          parallelList->post( pinfo->status, pintptr( pinfo->parallel ) );
        }
        else
        {
          pinfoparallel = new peer_info;
          ( (peer_info *) pinfo )->setParallel( pinfoparallel );

          if( parallelList && pinfoparallel )
            parallelList->post( pinfoparallel->status, pintptr( pinfoparallel ) );
        }
      }
    }
  }

  peer_lock.unlock();
}
