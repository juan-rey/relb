/*
  AdminHTTPServer.cpp: Web Admin class source file.

   Copyright 2006, 2007, 2008, 2009 Juan Rey Saura

This file is part of RELB Easy Load Balancer.

This software is a copyrighted work licensed under the Open Software License version 3.0
Please consult the file "LICENSE.txt" for details.
*/

#include "AdminHTTPServer.h"

#include <stdio.h>
#include <stdlib.h>

#ifdef WIN32
#  include <winsock2.h>
#else
#  include <sys/time.h>
#  include <sys/types.h>
#  include <sys/socket.h>
#  include <netinet/in.h>
#  include <arpa/inet.h>
#  include <netdb.h>
#  include <unistd.h>
#  include <signal.h>
#  include <time.h>
#endif

#include "utiles.h"
#include <pasync.h>
#include "htmldefs.h"
#include "relb.h"

USING_PTYPES


#define SORT_BY_SRC_IP 1
#define SORT_BY_DST_IP 2
#define SORT_BY_STATUS 3
#define SORT_BY_MODIFIED 4
#define SORT_BY_CREATION 5

#define SORT_BY_MIN 1
#define SORT_BY_MAX 5

#define TEXT_SORT_BY_SRC_IP "src"
#define TEXT_SORT_BY_DST_IP "dst"
#define TEXT_SORT_BY_STATUS "stts"
#define TEXT_SORT_BY_MODIFIED "mod"
#define TEXT_SORT_BY_CREATION "creat"

#define TEXT_STATUS_PARAM "status="
#define TEXT_SORT_BY_PARAM "sort_by="
#define TEXT_SORT_ASC_PARAM "sort_asc=false"
#define TEXT_SRC_IP_PARAM "src_ip="
#define TEXT_SRC_PORT_PARAM "src_port="
#define TEXT_DST_IP_PARAM "dst_ip="
#define TEXT_DST_PORT_PARAM "dst_port="

#define WEB_PAGE_LIST  "list"
#define WEB_PAGE_SERVERS  "servers"
#define WEB_PAGE_BAN  "ban"
#define WEB_PAGE_BAN_ALL  "ban_all"
#define WEB_PAGE_KICK  "kick"
#define WEB_PAGE_REDIRECT  "redirect"
#define WEB_PAGE_DEFAULT  WEB_PAGE_SERVERS


AdminHTTPServer::AdminHTTPServer(): thread( false )
{
  finish = false;
  parallellist_jq = NULL;
}

AdminHTTPServer::~AdminHTTPServer()
{
  message * msg = NULL;
  finish = true;
  waitfor();
  while( ( msg = jq.getmessage( 0 ) ) != NULL )
  {
    delete msg;
    msg = NULL;
  }

  peer_info * pinfo = NULL;

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

  cleanServersList();
}

void AdminHTTPServer::startHTTPAdmin( unsigned short port, const ipaddress ip )
{
  bind_port = port;
  bind_ip = ip;
  start();
}

void AdminHTTPServer::stopHTTPAdmin()
{
  finish = true;
}

bool AdminHTTPServer::addServer( const char * nombre, const char * hostname, unsigned short puerto, int weight, int max_connections )
{
  ipaddress ip = phostbyname( hostname );

  return addServer( nombre, &ip, puerto, weight );
}

bool AdminHTTPServer::addServer( const char * nombre, const ipaddress * ip, unsigned short puerto, int weight, int max_connections )
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
  server_list.add( info );

  return true;
}

void AdminHTTPServer::setPararllelList( const MessaggeQueue * jq )
{
  parallellist_jq = (MessaggeQueue *) jq;
}

const MessaggeQueue * AdminHTTPServer::getJQ()
{
  return &jq;
}


void AdminHTTPServer::execute()
{
  peer_info * pinfo = NULL;
  message * msg = NULL;
  ipstmserver server;
  ipstream client;
  char buff[4096 + 1];
  int  l = 0;
  bool default_redirection;

  int gg = server.bind( bind_ip, bind_port );
  int current_item_index = 0;
  while( !finish )
  {
    while( ( msg = jq.getmessage( 0 ) ) != NULL )
    {
      pinfo = NULL;
      if( msg->id != STATUS_CONNECTING_SERVER )
      {
        current_item_index = 0;
        while( !pinfo && current_item_index < peer_list.get_count() )
        {
          if( peer_list[current_item_index] == (peer_info *) msg->param )
          {
            pinfo = peer_list[current_item_index];
          }
          else
          {
            current_item_index++;
          }
        }

        if( pinfo != NULL )
        {
          pinfo->status = msg->id;
          pinfo->modified = NOW_UTC;
        }
      }
      else
      {
        pinfo = (peer_info *) msg->param;
        int peer_index = 0;

        while( peer_index < peer_list.get_count() )
        {
          //Lets clean disconnected connection from the same IP
          if( ( peer_list[peer_index]->src_ip == pinfo->src_ip ) && ( peer_list[peer_index]->status & STATUS_PEER_NOT_CONNECTED ) )
          {
            int server_index = 0;
            while( server_index < server_list.get_count() )
            {
              if( ( server_list[server_index]->ip == peer_list[peer_index]->dst_ip ) )
              {
                if( ( peer_list[peer_index]->status & STATUS_PEER_DISCONNECTED ) && ( server_list[server_index]->disconnected ) )
                  server_list[server_index]->disconnected = MAX( server_list[server_index]->disconnected - 1, 0 );
                else
                  server_list[server_index]->finished = MAX( server_list[server_index]->finished - 1, 0 );

                TRACE( TRACE_CONNECTIONS )( "%s - AdminHttp - server %d, connecting %d, connected %d, finished %d, disconnected %d, total %d connections\n", curr_local_time(), server_index, server_list[server_index]->connecting, server_list[server_index]->connected, server_list[server_index]->finished, server_list[server_index]->disconnected, server_list[server_index]->connecting + server_list[server_index]->connected + server_list[server_index]->finished + server_list[server_index]->disconnected );

                server_index = server_list.get_count();
              }
              server_index++;
            }

            peer_list.del( peer_index );
          }
          else
          {
            peer_index++;
          }
        }

        peer_list.add( pinfo );
      }

      if( pinfo != NULL )
      {
        switch( msg->id & STATUS_CLIENT_SERVER_MASK )
        {
          case STATUS_CONNECTION_LOST:
          case STATUS_DISCONNECTED_OK:
          case STATUS_CONNECTION_FAILED:
          case STATUS_PEER_CONNECTION_DELETED:
          {
            int i = 0;
            int peer_index = 0;
            bool hay_mas_conectados = false;

            if( msg->id == STATUS_PEER_CONNECTION_DELETED )
              hay_mas_conectados = true;//borro seguro

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

            while( i < server_list.get_count() )
            {
              if( ( server_list[i]->ip == pinfo->dst_ip ) && ( server_list[i]->port == pinfo->dst_port ) )
              {
                //server_list[i]->connected = max( server_list[i]->connected - 1, 0);
                if( msg->id == STATUS_CONNECTION_FAILED )
                {
                  server_list[i]->last_connection_failed = true;
                  server_list[i]->last_connection_attempt = NOW_UTC;
                }

                if( msg->id & STATUS_CONNECTION_FAILED )
                  server_list[i]->connecting = MAX( server_list[i]->connecting - 1, 0 );
                else
                  server_list[i]->connected = MAX( server_list[i]->connected - 1, 0 );

                if( hay_mas_conectados && current_item_index > -1 && current_item_index < peer_list.get_count() )
                {
                  pinfo = NULL;
                  peer_list.del( current_item_index );
                }
                else
                {
                  if( msg->id & STATUS_PEER_DISCONNECTED )
                    server_list[i]->disconnected++;
                  else
                    server_list[i]->finished++;
                }

                TRACE( TRACE_CONNECTIONS )( "%s - AdminHttp - server %d, connecting %d, connected %d, finished %d, disconnected %d, total %d connections\n", curr_local_time(), i, server_list[i]->connecting, server_list[i]->connected, server_list[i]->finished, server_list[i]->disconnected, server_list[i]->connecting + server_list[i]->connected + server_list[i]->finished + server_list[i]->disconnected );
                i = server_list.get_count();
              }
              i++;
            }
          }
          break;
          case STATUS_CONNECTION_ESTABLISHED:
          {
            int i = 0;
            while( i < server_list.get_count() )
            {
              if( ( server_list[i]->ip == pinfo->dst_ip ) && ( server_list[i]->port == pinfo->dst_port ) )
              {
                server_list[i]->connected++;
                server_list[i]->connecting = MAX( server_list[i]->connecting - 1, 0 );
                server_list[i]->last_connection_failed = false;
                server_list[i]->last_connection_attempt = NOW_UTC;
                TRACE( TRACE_CONNECTIONS )( "%s - AdminHttp - server %d, connecting %d, connected %d, finished %d, disconnected %d, total %d connections\n", curr_local_time(), i, server_list[i]->connecting, server_list[i]->connected, server_list[i]->finished, server_list[i]->disconnected, server_list[i]->connecting + server_list[i]->connected + server_list[i]->finished + server_list[i]->disconnected );
                i = server_list.get_count();
              }
              i++;
            }
          }
          break;
          case STATUS_CONNECTING_SERVER:
          {
            int i = 0;

            while( i < server_list.get_count() )
            {
              if( ( server_list[i]->ip == pinfo->dst_ip ) && ( server_list[i]->port == pinfo->dst_port ) )
              {
                server_list[i]->connecting++;
                TRACE( TRACE_CONNECTIONS )( "%s - AdminHttp - server %d, connecting %d, connected %d, finished %d, disconnected %d, total %d connections\n", curr_local_time(), i, server_list[i]->connecting, server_list[i]->connected, server_list[i]->finished, server_list[i]->disconnected, server_list[i]->connecting + server_list[i]->connected + server_list[i]->finished + server_list[i]->disconnected );
                i = server_list.get_count();
              }
              i++;
            }
          }
          break;
        }

      }

      delete msg;
      msg = NULL;
    }

    //server.poll();
    if( server.serve( client, gg, 1000 ) )
    {
      int i = 0;
      default_redirection = true;
      buff[0] = 1;
      while( ( l = client.line( buff, 4096 ) ) > 0 )
      {
        buff[l] = 0;
        if( strncmp( buff, "GET ", 4 ) == 0 )
        {
          i = 5;
          while( i < l )
          {
            if( ( buff[i] == ' ' ) || ( buff[i] == '&' ) || ( buff[i] == '?' ) )
              buff[i] = 0;
            i++;
          }

          TRACE( TRACE_CONNECTIONS )( "%s - Request was %s\n", curr_local_time(), buff + 4 );

          if( strcmp( buff + 5, WEB_PAGE_LIST ) == 0 )
          {
            ipaddress src_ip_filter = ipnone;
            ipaddress dst_ip_filter = ipnone;
            int status_filter = 0;
            int sort_by = 1;
            unsigned short src_port = 0;
            unsigned short dst_port = 0;
            bool sort_ascending = true;
            default_redirection = false;

            i = 5;
            while( i < l )
            {
              if( buff[i] == 0 )
              {
                if( strncmp( buff + i + 1, TEXT_SRC_IP_PARAM, strlen( TEXT_SRC_IP_PARAM ) ) == 0 )
                  src_ip_filter = chartoipaddress( buff + i + strlen( TEXT_SRC_IP_PARAM ) + 1 );

                if( strncmp( buff + i + 1, TEXT_SRC_PORT_PARAM, strlen( TEXT_SRC_PORT_PARAM ) ) == 0 )
                  src_port = atoi( buff + i + strlen( TEXT_SRC_PORT_PARAM ) + 1 );

                if( strncmp( buff + i + 1, TEXT_DST_IP_PARAM, strlen( TEXT_DST_IP_PARAM ) ) == 0 )
                  dst_ip_filter = chartoipaddress( buff + i + strlen( TEXT_DST_IP_PARAM ) + 1 );

                if( strncmp( buff + i + 1, TEXT_DST_PORT_PARAM, strlen( TEXT_DST_PORT_PARAM ) ) == 0 )
                  dst_port = atoi( buff + i + strlen( TEXT_DST_PORT_PARAM ) + 1 );

                if( strncmp( buff + i + 1, TEXT_STATUS_PARAM, strlen( TEXT_STATUS_PARAM ) ) == 0 )
                  status_filter = atoi( buff + i + strlen( TEXT_STATUS_PARAM ) + 1 );

                if( strncmp( buff + i + 1, TEXT_SORT_BY_PARAM, strlen( TEXT_SORT_BY_PARAM ) ) == 0 )
                {
                  if( strcmp( buff + i + strlen( TEXT_SORT_BY_PARAM ) + 1, TEXT_SORT_BY_SRC_IP ) == 0 )
                  {
                    sort_by = SORT_BY_SRC_IP;
                  }

                  if( strcmp( buff + i + strlen( TEXT_SORT_BY_PARAM ) + 1, TEXT_SORT_BY_DST_IP ) == 0 )
                  {
                    sort_by = SORT_BY_DST_IP;
                  }

                  if( strcmp( buff + i + strlen( TEXT_SORT_BY_PARAM ) + 1, TEXT_SORT_BY_STATUS ) == 0 )
                  {
                    sort_by = SORT_BY_STATUS;
                  }

                  if( strcmp( buff + i + strlen( TEXT_SORT_BY_PARAM ) + 1, TEXT_SORT_BY_MODIFIED ) == 0 )
                  {
                    sort_by = SORT_BY_MODIFIED;
                  }

                  if( strcmp( buff + i + strlen( TEXT_SORT_BY_PARAM ) + 1, TEXT_SORT_BY_CREATION ) == 0 )
                  {
                    sort_by = SORT_BY_CREATION;
                  }

                  if( atoi( buff + i + strlen( TEXT_SORT_BY_PARAM ) + 1 ) > 0 )
                    sort_by = atoi( buff + i + strlen( TEXT_SORT_BY_PARAM ) + 1 );
                }

                if( strncmp( buff + i + 1, TEXT_SORT_ASC_PARAM, strlen( TEXT_SORT_ASC_PARAM ) ) == 0 )
                  sort_ascending = false;
              }
              i++;
            }
            //lista conexiones
            list( client, src_ip_filter, dst_ip_filter, src_port, dst_port, status_filter, sort_by, sort_ascending );
          }


          if( strcmp( buff + 5, WEB_PAGE_BAN ) == 0 )
          {
            char * src_ip = NULL;
            char * src_port = NULL;
            i = 5;
            while( i < l )
            {
              if( buff[i] == 0 )
              {
                if( strncmp( buff + i + 1, TEXT_SRC_IP_PARAM, strlen( TEXT_SRC_IP_PARAM ) ) == 0 )
                  src_ip = buff + i + strlen( TEXT_SRC_IP_PARAM ) + 1;

                if( strncmp( buff + i + 1, TEXT_SRC_PORT_PARAM, strlen( TEXT_SRC_PORT_PARAM ) ) == 0 )
                  src_port = buff + i + strlen( TEXT_SRC_PORT_PARAM ) + 1;

              }
              i++;
            }

            default_redirection = false;
            ban( client, src_ip, src_port );
          }

          if( strcmp( buff + 5, WEB_PAGE_KICK ) == 0 )
          {
            char * src_ip = NULL;
            char * src_port = NULL;
            i = 5;
            while( i < l )
            {
              if( buff[i] == 0 )
              {
                if( strncmp( buff + i + 1, TEXT_SRC_IP_PARAM, strlen( TEXT_SRC_IP_PARAM ) ) == 0 )
                  src_ip = buff + i + strlen( TEXT_SRC_IP_PARAM ) + 1;

                if( strncmp( buff + i + 1, TEXT_SRC_PORT_PARAM, strlen( TEXT_SRC_PORT_PARAM ) ) == 0 )
                  src_port = buff + i + strlen( TEXT_SRC_PORT_PARAM ) + 1;
              }
              i++;
            }
            default_redirection = false;
            kick( client, src_ip, src_port );
          }

          if( strcmp( buff + 5, WEB_PAGE_BAN_ALL ) == 0 )
          {
            char * src_ip = NULL;
            char * src_port = NULL;
            i = 5;
            while( i < l )
            {
              if( buff[i] == 0 )
              {
                if( strncmp( buff + i + 1, TEXT_SRC_IP_PARAM, strlen( TEXT_SRC_IP_PARAM ) ) == 0 )
                  src_ip = buff + i + strlen( TEXT_SRC_IP_PARAM ) + 1;

                if( strncmp( buff + i + 1, TEXT_SRC_PORT_PARAM, strlen( TEXT_SRC_PORT_PARAM ) ) == 0 )
                  src_port = buff + i + strlen( TEXT_SRC_PORT_PARAM ) + 1;
              }
              i++;
            }
            default_redirection = false;
            ban_all( client, src_ip, src_port );
          }

          if( strcmp( buff + 5, WEB_PAGE_SERVERS ) == 0 )
          {
            default_redirection = false;
            servers( client );
          }

          if( default_redirection )
          {
            redirect( client );
          }
        }
      }
      client.close();
      //Content-Length: xxxx
    }
  }
}



void AdminHTTPServer::cleanup()
{

}

void AdminHTTPServer::cleanServersList()
{
  const serverinfo * pinfo = NULL;

  while( server_list.get_count() )
  {
    pinfo = server_list[0];

    if( pinfo != NULL )
    {
      if( pinfo->nombre )
        delete pinfo->nombre;
      delete pinfo;
    }
    server_list.del( 0 );
    pinfo = NULL;
  }
}
void AdminHTTPServer::list( ipstream & client, ipaddress src_filter, ipaddress dst_filter, unsigned short src_port, unsigned short dst_port, int status_filter, int sort_by, bool ascending )
{
  int hours, mins, secs, msecs, year, month, day;
  int i, j;
  tpodlist<peer_info *> * pList = NULL;
  string filters;

  client.putline( "HTTP/1.1 200 OK" );
  client.putline( "Content-Type: text/html" );
  client.puteol();
  client.putline( "<html><head><title>Administration</title>" );
  client.putline( "<meta http-equiv=\"refresh\" content=\"10\">" );
  client.write( HTML_STYLE_SECTION, strlen( HTML_STYLE_SECTION ) );
  client.putline( "</head><body>" );
  client.putline( string( "<span class=\"rwaTitle\">" ) + APP_FULL_NAME + " " + APP_VERSION + "</span><br><p>" );
  client.putline( "<span class=\"rwaTitle\">Web Admin</span><br><p>" );
  client.putline( "<span class=\"rwaBody\"> ? Active connections. " + itostring( peer_list.get_count() ) + " Cached connections.</span><br><p>" );
  decodedate( now( false ), year, month, day );
  decodetime( now( false ), hours, mins, secs, msecs );
  client.putf( "<span class=\"rwaBody\"> %04d-%02d-%02d %02d:%02d:%02d,%03d ", year, month, day, hours, mins, secs, msecs );
  client.putline( "</span><br><p>" );
  client.putf( string( "<span class=\"rwaBody\"><a href=\"" ) + WEB_PAGE_SERVERS + "\">all servers</a>  <a href=\"" + WEB_PAGE_LIST + "\">all connections</a></span><br><p>", year, month, day, hours, mins, secs, msecs );
  client.write( HTML_TABLE_START, strlen( HTML_TABLE_START ) );
  client.putline( "<tr>\n" );
  filters = ( ( src_filter != ipnone ) ? ( string( TEXT_SRC_IP_PARAM ) + iptostring( src_filter ) + "&" ) : ( string( "" ) ) )
    + ( ( src_port != 0 ) ? ( string( TEXT_DST_PORT_PARAM ) + itostring( src_port ) + "&" ) : ( string( "" ) ) )
    + ( ( dst_filter != ipnone ) ? ( string( TEXT_DST_IP_PARAM ) + iptostring( dst_filter ) + "&" ) : ( string( "" ) ) )
    + ( ( dst_port != 0 ) ? ( string( TEXT_DST_PORT_PARAM ) + itostring( dst_port ) + "&" ) : ( string( "" ) ) )
    + ( ( status_filter > 0 ) ? ( string( TEXT_STATUS_PARAM ) + string( status_filter ) + "&" ) : ( string( "" ) ) );
  client.putline( "<th class=\"rwaTableHeader\">source (<a href=\"list?"
    + filters + TEXT_SORT_BY_PARAM + TEXT_SORT_BY_SRC_IP
    //      + ((sort_by>0)?(string(TEXT_SORT_BY_PARAM) + string(sort_by)):(string("")))                      
    + "\">a</a>/<a href=\"" + WEB_PAGE_LIST + "?"
    + filters + TEXT_SORT_BY_PARAM + TEXT_SORT_BY_SRC_IP + "&" + TEXT_SORT_ASC_PARAM
    + "\">d</a>)</td>" );
  client.putline( "<th class=\"rwaTableHeader\">destination (<a href=\"list?"
    + filters + TEXT_SORT_BY_PARAM + TEXT_SORT_BY_DST_IP
    + "\">a</a>/<a href=\"" + WEB_PAGE_LIST + "?"
    + filters + TEXT_SORT_BY_PARAM + TEXT_SORT_BY_DST_IP + "&" + TEXT_SORT_ASC_PARAM
    + "\">d</a>)</td>" );
  client.putline( "<th class=\"rwaTableHeader\">status (<a href=\"list?"
    + filters + TEXT_SORT_BY_PARAM + TEXT_SORT_BY_STATUS
    + "\">a</a>/<a href=\"" + WEB_PAGE_LIST + "?"
    + filters + TEXT_SORT_BY_PARAM + TEXT_SORT_BY_STATUS + "&" + TEXT_SORT_ASC_PARAM
    + "\">d</a>)</td>" );
  client.putline( "<th class=\"rwaTableHeader\">duration (<a href=\"list?"
    + filters + TEXT_SORT_BY_PARAM + TEXT_SORT_BY_MODIFIED
    + "\">a</a>/<a href=\"" + WEB_PAGE_LIST + "?"
    + filters + TEXT_SORT_BY_PARAM + TEXT_SORT_BY_MODIFIED + "&" + TEXT_SORT_ASC_PARAM
    + "\">d</a>)</td>" );
  client.putline( "<th class=\"rwaTableHeader\">registred at (<a href=\"list?"
    + filters + TEXT_SORT_BY_PARAM + TEXT_SORT_BY_CREATION
    + "\">a</a>/<a href=\"" + WEB_PAGE_LIST + "?"
    + filters + TEXT_SORT_BY_PARAM + TEXT_SORT_BY_CREATION + "&" + TEXT_SORT_ASC_PARAM
    + "\">d</a>)</td>" );
  client.putline( "<th width=\"30px\" class=\"rwaTableHeader\">kick</td>" );
  client.putline( "<th width=\"90px\" class=\"rwaTableHeader\">ban con. to</td>" );
  client.putline( "</tr>" );

  if( sort_by != 0 )
  {
    peer_info * pInfo = NULL;
    //copiar lista ordenada
    pList = new tpodlist<peer_info *>;
    bool inserthere = false;
    pInfo = NULL;
    i = 0;

    while( i < peer_list.get_count() )
    {
      bool filterok = true;

      if( src_filter != ipnone && peer_list[i]->src_ip != src_filter )
        filterok = false;

      if( src_port != 0 && peer_list[i]->src_port != src_port )
        filterok = false;

      if( dst_filter != ipnone && peer_list[i]->dst_ip != dst_filter )
        filterok = false;

      if( dst_port != 0 && peer_list[i]->dst_port != dst_port )
        filterok = false;

      if( status_filter != 0 && !( peer_list[i]->status & status_filter ) )
        filterok = false;

      if( filterok )
      {
        pInfo = new peer_info();
        pInfo->src_ip = peer_list[i]->src_ip;
        pInfo->src_port = peer_list[i]->src_port;
        pInfo->dst_ip = peer_list[i]->dst_ip;
        pInfo->dst_port = peer_list[i]->dst_port;
        pInfo->status = peer_list[i]->status;
        pInfo->created = peer_list[i]->created;
        pInfo->modified = peer_list[i]->modified;
        pInfo->parallel = peer_list[i]->parallel;
        j = 0;
        inserthere = false;

        while( ( j < pList->get_count() ) && ( !inserthere ) )
        {
          if( ascending )
          {
            switch( sort_by )
            {
              case SORT_BY_SRC_IP:
              {
                if( ipmenor( &( pInfo->src_ip ), &( ( *pList )[j]->src_ip ) ) )
                  inserthere = true;
                else
                  if( ( pInfo->src_ip == ( *pList )[j]->src_ip ) && ( pInfo->src_port < ( *pList )[j]->src_port ) )
                    inserthere = true;
              }
              break;
              case SORT_BY_DST_IP:
              {
                if( ipmenor( &( pInfo->dst_ip ), &( ( *pList )[j]->dst_ip ) ) )
                  inserthere = true;
                else
                  if( ( pInfo->dst_ip == ( *pList )[j]->dst_ip ) && ( pInfo->dst_port < ( *pList )[j]->dst_port ) )
                    inserthere = true;
              }
              break;
              case SORT_BY_STATUS:
                if( pInfo->status < ( *pList )[j]->status )
                {
                  inserthere = true;
                }
                break;
              case SORT_BY_MODIFIED:
                if( pInfo->modified >= ( *pList )[j]->modified )
                {
                  inserthere = true;
                }
                break;
              case SORT_BY_CREATION:
                if( pInfo->created < ( *pList )[j]->created )
                {
                  inserthere = true;
                }
                break;
            }
          }
          else
          {
            switch( sort_by )
            {
              case SORT_BY_SRC_IP:
              {
                if( !ipmenor( &( pInfo->src_ip ), &( ( *pList )[j]->src_ip ) ) && !( pInfo->src_ip == ( *pList )[j]->src_ip ) )
                  inserthere = true;
                else
                  if( ( pInfo->src_ip == ( *pList )[j]->src_ip ) && ( pInfo->src_port >= ( *pList )[j]->src_port ) )
                    inserthere = true;
              }
              break;
              case SORT_BY_DST_IP:
              {
                if( !ipmenor( &( pInfo->dst_ip ), &( ( *pList )[j]->dst_ip ) ) && !( pInfo->dst_ip == ( *pList )[j]->dst_ip ) )
                  inserthere = true;
                else
                  if( ( pInfo->dst_ip == ( *pList )[j]->dst_ip ) && ( pInfo->dst_port >= ( *pList )[j]->dst_port ) )
                    inserthere = true;
              }
              break;
              case SORT_BY_STATUS:
                if( pInfo->status >= ( *pList )[j]->status )
                {
                  inserthere = true;
                }
                break;
              case SORT_BY_MODIFIED:
                if( pInfo->modified < ( *pList )[j]->modified )
                {
                  inserthere = true;
                }
                break;
              case SORT_BY_CREATION:
                if( pInfo->created >= ( *pList )[j]->created )
                {
                  inserthere = true;
                }
                break;
            }
          }
          j++;
        }

        if( inserthere )
        {
          pList->ins( j - 1, pInfo );
        }
        else
        {
          pList->add( pInfo );

        }
      }

      pInfo = NULL;
      i++;
    }
  }
  else
  {
    pList = &peer_list;
  }

  i = 0;
  while( i < pList->get_count() )
  {
    client.putline( "<tr class=\"rwaBody\">" );
    client.putline( "<td align=\"center\">" + iptostring( ( *pList )[i]->src_ip ) + ":" + itostring( ( *pList )[i]->src_port ) + "</td>" );
    client.putline( "<td align=\"center\">" + iptostring( ( *pList )[i]->dst_ip ) + ":" + itostring( ( *pList )[i]->dst_port ) + "</td>" );
    client.putline( "<td align=\"center\">" + string( statusdesc( ( *pList )[i]->status ) ) + "</td>" );
    decodetime( NOW_UTC - ( *pList )[i]->modified, hours, mins, secs, msecs );
    client.putf( "<td align=\"center\"> %02d:%02d:%02d ", hours, mins, secs );
    client.putline( "</td align=\"center\">" );
    decodedate( ( *pList )[i]->created + tzoffset() * 60000, year, month, day );
    decodetime( ( *pList )[i]->created + tzoffset() * 60000, hours, mins, secs, msecs );
    client.putf( "<td align=\"center\"> %04d-%02d-%02d %02d:%02d:%02d,%03d ", year, month, day, hours, mins, secs, msecs );
    client.putline( "</td>" );
    client.putline( "<td align=\"center\"><a href=\"" + string( WEB_PAGE_KICK ) + "?"
      + TEXT_SRC_IP_PARAM + iptostring( ( *pList )[i]->src_ip ) + "&" + TEXT_SRC_PORT_PARAM + itostring( ( *pList )[i]->src_port ) +
      +"\">X</a></td>" );
    client.putline( "<td align=\"center\"><a href=\"" + string( WEB_PAGE_BAN ) + "?"
      + TEXT_SRC_IP_PARAM + iptostring( ( *pList )[i]->src_ip ) + "&" + TEXT_SRC_PORT_PARAM + itostring( ( *pList )[i]->src_port ) +
      +"\">THIS</a>  -   <a href=\"" + string( WEB_PAGE_BAN_ALL ) + "?"
      + TEXT_SRC_IP_PARAM + iptostring( ( *pList )[i]->src_ip ) + "&" + TEXT_SRC_PORT_PARAM + itostring( ( *pList )[i]->src_port ) +
      +"\">ALL</a></td>" );
    client.putline( "</tr>" );
    i++;
  }

  if( sort_by != 0 )
  {
    while( pList->get_count() )
    {
      delete ( *pList )[0];
      pList->del( 0 );
    }
    delete pList;
  }

  pList = NULL;

  client.write( HTML_TABLE_END, strlen( HTML_TABLE_END ) );
  client.putline( "</body></html>" );
  client.putline( "" );

}



void AdminHTTPServer::ban( ipstream & client, char * src_ip, char * src_port, bool undo )
{
  bool success = false;
  ipaddress ip;
  unsigned short port;

  ip = chartoipaddress( src_ip );
  port = atoi( src_port );

  if( parallellist_jq )
  {
    int i = 0;
    peer_info * pinfo = NULL;

    while( i < peer_list.get_count() && !pinfo )
    {
      if( peer_list[i]->src_ip == ip && peer_list[i]->src_port == port )
        pinfo = peer_list[i]->parallel;
      i++;
    }

    if( parallellist_jq && pinfo )
    {
      parallellist_jq->post( STATUS_PEER_CONNECTION_BAN_THIS, pintptr( pinfo ) );
      success = true;
    }
  }

  client.putline( "HTTP/1.1 200 OK" );
  client.putline( "Content-Type: text/html" );
  client.puteol();
  client.putline( "<html><head><title>Admin</title>" );
  client.putline( "<meta http-equiv=\"refresh\" content=\"10;url=" + string( WEB_PAGE_DEFAULT ) + "\">" );
  client.write( HTML_STYLE_SECTION, strlen( HTML_STYLE_SECTION ) );
  client.putline( "</head><body>" );
  client.putline( "<span class=\"rwaTitle\">Web Admin</span><br><p>" );
  client.putline( string( "<span class=\"rwaBody\">ban" ) + src_ip + ":" + src_port + "</span><br><p>" );
  if( success )
    client.putline( "<span class=\"rwaBody\">OK</span><br><p>" );
  else
    client.putline( "<span class=\"rwaBody\">KO</span><br><p>" );
  client.putline( "</body></html>" );
  client.putline( "" );
}

void AdminHTTPServer::ban_all( ipstream & client, char * src_ip, char * src_port, bool undo )
{
  bool success = false;
  ipaddress ip;
  unsigned short port;

  ip = chartoipaddress( src_ip );
  port = atoi( src_port );

  if( parallellist_jq )
  {
    int i = 0;
    peer_info * pinfo = NULL;

    while( i < peer_list.get_count() && !pinfo )
    {
      if( peer_list[i]->src_ip == ip && peer_list[i]->src_port == port )
        pinfo = peer_list[i]->parallel;
      i++;
    }

    if( parallellist_jq && pinfo )
    {
      parallellist_jq->post( STATUS_PEER_CONNECTION_BAN_ALL, pintptr( pinfo ) );
      success = true;
    }
  }
  client.putline( "HTTP/1.1 200 OK" );
  client.putline( "Content-Type: text/html" );
  client.puteol();
  client.putline( "<html><head><title>Admin</title>" );
  client.putline( "<meta http-equiv=\"refresh\" content=\"10;url=" + string( WEB_PAGE_DEFAULT ) + "\">" );
  client.write( HTML_STYLE_SECTION, strlen( HTML_STYLE_SECTION ) );
  client.putline( "</head><body>" );
  client.putline( "<span class=\"rwaTitle\">Web Admin</span><br><p>" );
  client.putline( string( "<span class=\"rwaBody\">ban_all" ) + src_ip + ":" + src_port + "</span><br><p>" );
  if( success )
    client.putline( "<span class=\"rwaBody\">OK</span><br><p>" );
  else
    client.putline( "<span class=\"rwaBody\">KO</span><br><p>" );
  client.putline( "</body></html>" );
  client.putline( "" );
}

void AdminHTTPServer::kick( ipstream & client, char * src_ip, char * src_port )
{
  bool success = false;
  ipaddress ip;
  unsigned short port;

  ip = chartoipaddress( src_ip );
  port = atoi( src_port );

  if( parallellist_jq )
  {
    int i = 0;
    peer_info * pinfo = NULL;

    while( i < peer_list.get_count() && !pinfo )
    {
      if( peer_list[i]->src_ip == ip && peer_list[i]->src_port == port )
        pinfo = peer_list[i]->parallel;
      i++;
    }

    if( parallellist_jq && pinfo )
    {
      parallellist_jq->post( STATUS_PEER_CONNECTION_KICKED, pintptr( pinfo ) );
      success = true;
    }
  }

  client.putline( "HTTP/1.1 200 OK" );
  client.putline( "Content-Type: text/html" );
  client.puteol();
  client.putline( "<html><head><title>Admin</title>" );
  client.putline( "<meta http-equiv=\"refresh\" content=\"10;url=" + string( WEB_PAGE_DEFAULT ) + "\">" );
  client.write( HTML_STYLE_SECTION, strlen( HTML_STYLE_SECTION ) );
  client.putline( "</head><body>" );
  client.putline( "<span class=\"rwaTitle\">Web Admin</span><br><p>" );
  client.putline( string( "<span class=\"rwaBody\">kick" ) + src_ip + ":" + src_port + "</span><br><p>" );
  if( success )
    client.putline( "<span class=\"rwaBody\">OK</span><br><p>" );
  else
    client.putline( "<span class=\"rwaBody\">KO</span><br><p>" );
  client.putline( "</body></html>" );
  client.putline( "" );
}

void AdminHTTPServer::servers( ipstream & client )
{
  int hours, mins, secs, msecs, year, month, day;
  int i = 0;
  client.putline( "HTTP/1.1 200 OK" );
  client.putline( "Content-Type: text/html" );
  client.puteol();
  client.putline( "<html><head><title>Admin</title>" );
  client.putline( "<meta http-equiv=\"refresh\" content=\"10;url=" + string( WEB_PAGE_DEFAULT ) + "\">" );
  client.write( HTML_STYLE_SECTION, strlen( HTML_STYLE_SECTION ) );
  client.putline( "</head><body>" );
  client.putline( string( "<span class=\"rwaTitle\">" ) + APP_FULL_NAME + " " + APP_VERSION + "</span><br><p>" );
  client.putline( "<span class=\"rwaTitle\">Web Admin </span><br><p>" );
  client.putline( "<span class=\"rwaBody\">servers</span><br><p>" );
  decodedate( NOW_LOCALTIME, year, month, day );
  decodetime( NOW_LOCALTIME, hours, mins, secs, msecs );
  client.putf( "<span class=\"rwaBody\"> %04d-%02d-%02d %02d:%02d:%02d,%03d ", year, month, day, hours, mins, secs, msecs );
  client.putline( "</span><br><p>" );
  client.putf( "<span class=\"rwaBody\"><a href=\"servers\">all servers</a>  <a href=\"list\">all connections</a></span><br><p>", year, month, day, hours, mins, secs, msecs );

  client.write( HTML_TABLE_START, strlen( HTML_TABLE_START ) );
  client.putline( "<tr>\n" );
  client.putline( "<th class=\"rwaTableHeader\">server ip</td>" );
  client.putline( "<th width=\"90px\" class=\"rwaTableHeader\">connected</td>" );
  client.putline( "<th width=\"90px\" class=\"rwaTableHeader\">disconnected</td>" );
  client.putline( "</tr>" );
  while( i < server_list.get_count() )
  {
    client.putline( "<tr class=\"rwaBody\">" );
    client.putline( "<td align=\"center\"><a href=\"list?" + string( TEXT_DST_IP_PARAM ) + iptostring( server_list[i]->ip )
      + "&" + TEXT_DST_PORT_PARAM + itostring( server_list[i]->port )
      + "\">" + iptostring( server_list[i]->ip ) + ":" + itostring( server_list[i]->port ) + "</a></td>" );
    client.putline( "<td align=\"center\">" + itostring( server_list[i]->connected ) + "</td>" );
    client.putline( "<td align=\"center\">" + itostring( server_list[i]->disconnected ) + "</td>" );
    client.putline( "</tr>" );
    i++;
  }
  client.write( HTML_TABLE_END, strlen( HTML_TABLE_END ) );
  client.putline( "</body></html>" );
  client.putline( "" );
}


void AdminHTTPServer::redirect( ipstream & client )
{
  client.putline( "HTTP/1.1 200 OK" );
  client.putline( "Content-Type: text/html" );
  client.puteol();
  client.putline( "<html><head><title>Admin</title>" );
  client.putline( "<meta http-equiv=\"refresh\" content=\"0;url=" + string( WEB_PAGE_DEFAULT ) + "\">" );
  client.putline( "</head><body>" );
  client.putline( "</body></html>" );
  client.putline( "" );
}

