/*
   ConnectionPeer.cpp: connection peer class source file.

   Copyright 2006, 2007, 2008, 2009 Juan Rey Saura

This file is part of RELB Easy Load Balancer..

This software is a copyrighted work licensed under the Open Software License version 3.0
Please consult the file "LICENSE.txt" for details.
*/

#include "ConnectionPeer.h"

#include <stdio.h>
#include "utiles.h"

//Testing option
//disable it if unsure
//#define OPTIMIZE_PEER_READING 

ConnectionPeer::ConnectionPeer( Socket * pcli, Socket * psrv, const MessaggeQueue * jq, const peer_info * info )
{
  pClient = pcli;
  pServer = psrv;
  w_offset_bufferforserver = 0;
  r_offset_bufferforserver = 0;
  bytes_to_receive_in_bufferforserver = STAMBUFFERSOCKET;
  bytes_to_send_in_bufferforserver = 0;
  w_offset_bufferforclient = 0;
  r_offset_bufferforclient = 0;
  bytes_to_receive_in_bufferforclient = STAMBUFFERSOCKET;
  bytes_to_send_in_bufferforclient = 0;
  pjq = (MessaggeQueue *) jq;
  pinfo = info;
  pServerConnected = false;
}

ConnectionPeer::~ConnectionPeer()
{
  close();

  if( pClient )
  {
    delete pClient;
  }

  if( pServer )
  {
    delete pServer;
  }
}

void ConnectionPeer::addToFDSETR( fd_set * set )
{
  TRACE( TRACE_VERY_VERBOSE )( "%s - pClient was %p and %d bytes free\n", curr_local_time(), (void *) pClient, bytes_to_receive_in_bufferforserver );

  if( pClient && bytes_to_receive_in_bufferforserver )
  {
    pClient->addToFDSET( set );
  }

  TRACE( TRACE_VERY_VERBOSE )( "%s - pServer was %p and %d bytes free\n", curr_local_time(), (void *) pServer, bytes_to_receive_in_bufferforclient );

  if( pServer && bytes_to_receive_in_bufferforclient )
  {
    pServer->addToFDSET( set );
  }
}

void ConnectionPeer::addToFDSETW( fd_set * set )
{
  if( pClient && bytes_to_send_in_bufferforclient )
  {
    pClient->addToFDSET( set );
  }

  if( pServer && bytes_to_send_in_bufferforserver )
  {
    pServer->addToFDSET( set );
  }
}

void ConnectionPeer::addToFDSETC( fd_set * setr, fd_set * setw )
{

  if( pClient && bytes_to_receive_in_bufferforserver )
  {
    TRACE( TRACE_CONNECTIONS )( "%s - Checking ongoing connection A\n", curr_local_time() );
    pClient->addToFDSET( setr );
  }


  if( pServer )
  {
    TRACE( TRACE_CONNECTIONS )( "%s - Checking ongoing connection B\n", curr_local_time() );
    pServer->addToFDSET( setw );
  }
}

void ConnectionPeer::closePInfo( peer_info * pcheck )
{
  if( pcheck == pinfo )
  {
    close();
    if( pjq && pinfo )
    {
      pjq->post( STATUS_SERVER_DISCONNECTED_OK, pintptr( pinfo ) );
    }
  }
}

void ConnectionPeer::close()
{
  closeClient();
  closeServer();
}

void ConnectionPeer::closeClient()
{
  if( pClient )
  {
    TRACE( TRACE_CONNECTIONS )( "%s - Closing client\n", curr_local_time() );
    pClient->close();
    delete pClient;
    pClient = NULL;
    r_offset_bufferforclient = 0;
    w_offset_bufferforclient = 0;
    bytes_to_receive_in_bufferforclient = STAMBUFFERSOCKET;
    bytes_to_send_in_bufferforclient = 0;
    if( pServer && !bytes_to_send_in_bufferforserver )
    {
      closeServer();
    }
    else
    {
      if( !pServerConnected )
        closeServer();
    }
  }

  if( !isActive() )
  {
    TRACE( TRACE_VERBOSE )( "%s - Client and server are already closed C\n", curr_local_time() );
  }
}

#ifdef DEBUG
// just for testing purposes
void ConnectionPeer::closeServer2()
{
  if( pServer )
    pServer->close();
}

void ConnectionPeer::closeClient2()
{
  if( pClient )
    pClient->close();
}
void ConnectionPeer::closeServer3()
{
  if( pServer )
    pServer->close2();
}

void ConnectionPeer::closeClient3()
{
  if( pClient )
    pClient->close2();
}
#endif

void ConnectionPeer::closeServer()
{
  if( pServer )
  {
    TRACE( TRACE_CONNECTIONS )( "%s - Closing server\n", curr_local_time() );
    pServer->close();
    delete pServer;
    pServer = NULL;
    w_offset_bufferforserver = 0;
    r_offset_bufferforserver = 0;
    bytes_to_receive_in_bufferforserver = STAMBUFFERSOCKET;
    bytes_to_send_in_bufferforserver = 0;
    if( pClient && !bytes_to_send_in_bufferforclient )
    {
      closeClient();
    }
  }

  if( !isActive() )
  {
    TRACE( TRACE_VERBOSE )( "%s - Client and server are already closed S\n", curr_local_time() );
  }
}

bool ConnectionPeer::isActive()
{
  return ( ( pClient ) || ( pServer ) );
}

bool ConnectionPeer::manageConnectingPeer( fd_set * setr, fd_set * setw )
{
  bool ret = false;


  if( pClient && pServer )
  {
    if( pClient->isInFDSET( setr ) )
    {
      TRACE( TRACE_IOSOCKET )( "%s - reading from client\n", curr_local_time() );
      getMissingDataFromClient();
    }

    if( pServer )
    {
      if( pServer->isInFDSET( setw ) )
      {
        TRACE( TRACE_IOSOCKET )( "%s - connecting to server\n", curr_local_time() );
        pServerConnected = true;
        if( pjq && pinfo )
        {
          pjq->post( STATUS_CONNECTION_ESTABLISHED, pintptr( pinfo ) );
        }
        ret = true;
      }
    }
  }
  else
  {
    if( pClient )
    {
      if( pClient->isInFDSET( setw ) )
      {
        TRACE( TRACE_IOSOCKET )( "%s - i am able to write in client's socket\n", curr_local_time() );
        sendMissingDataToClient();
      }

      //nothing to send to the client
      if( !bytes_to_send_in_bufferforclient )
      {
        TRACE( TRACE_CONNECTIONS )( "%s - killing client, nothing to be sent form server\n", curr_local_time() );
        closeClient();
        if( pjq && pinfo )
        {
          pjq->post( STATUS_PEER_CONNECTION_CANCELLED, pintptr( pinfo ) );
        }
        ret = true;
      }
      else
      {
        ret = false;
      }
    }

    if( pServer )
    {
      TRACE( TRACE_CONNECTIONS )( "%s - killing server, nothing to be sent form client\n", curr_local_time() );
      closeServer();
      if( pjq && pinfo )
      {
        pjq->post( STATUS_PEER_CONNECTION_CANCELLED, pintptr( pinfo ) );
      }
      ret = false;
    }
  }

  return ret;
}

void ConnectionPeer::manageConnections( fd_set * setr, fd_set * setw )
{

  if( pClient && pServer )
  {
    TRACE( TRACE_VERY_VERBOSE )( "%s - Entro en manageConnections \n", curr_local_time() );
    if( pClient->isInFDSET( setr ) )
    {
      TRACE( TRACE_IOSOCKET )( "%s - reading from the client\n", curr_local_time() );
      getMissingDataFromClient();
#ifndef OPTIMIZE_PEER_READING 
      sendMissingDataToServer();
#endif
    }
#ifndef OPTIMIZE_PEER_READING 
    else
    {
#endif
      if( pServer->isInFDSET( setw ) )
      {
        TRACE( TRACE_IOSOCKET )( "%s - i can send to client\n", curr_local_time() );
        sendMissingDataToServer();
      }
#ifndef OPTIMIZE_PEER_READING 
    }
#endif

    if( pServer )
    {
      if( pServer->isInFDSET( setr ) )
      {
        TRACE( TRACE_IOSOCKET )( "%s - i am reading from server\n", curr_local_time() );

        getMissingDataFromServer();
#ifndef OPTIMIZE_PEER_READING 
        sendMissingDataToClient();
#endif
      }
#ifndef OPTIMIZE_PEER_READING 
      else
      {
#endif
        if( pClient->isInFDSET( setw ) )
        {
          TRACE( TRACE_IOSOCKET )( "%s - i can send to client\n", curr_local_time() );
          sendMissingDataToClient();
        }
#ifndef OPTIMIZE_PEER_READING 
      }
#endif 
    }
  }
  else
  {

    if( pClient )
    {
      if( pClient->isInFDSET( setw ) )
      {
        TRACE( TRACE_IOSOCKET )( "%s - i can send to client\n", curr_local_time() );
        sendMissingDataToClient();
      }
      //Si ya no queda nada que enviar al cliente
      if( !bytes_to_send_in_bufferforclient )
      {
        TRACE( TRACE_CONNECTIONS )( "%s - killing client, nothing to be sent to server\n", curr_local_time() );
        closeClient();
        if( pjq && pinfo )
        {
          pjq->post( STATUS_PEER_CONNECTION_CANCELLED, pintptr( pinfo ) );
        }
      }
    }

    if( pServer )
    {
      if( pServer->isInFDSET( setw ) )
      {
        TRACE( TRACE_IOSOCKET )( "%s - i can send to the server\n", curr_local_time() );
        sendMissingDataToServer();
      }

      if( !bytes_to_send_in_bufferforserver )
      {
        TRACE( TRACE_CONNECTIONS )( "%s - killing server, nothing else to send client \n", curr_local_time() );
        closeServer();
        if( pjq && pinfo )
        {
          pjq->post( STATUS_PEER_CONNECTION_CANCELLED, pintptr( pinfo ) );
        }
      }
    }

  }
}

void ConnectionPeer::getMissingDataFromClient()
{
  int ret;

  if( pClient && bytes_to_receive_in_bufferforserver )
  {
    ret = pClient->recieveNB( &bufferforserver[w_offset_bufferforserver], bytes_to_receive_in_bufferforserver );
    if( ret > 0 )
    {
      TRACE( TRACE_IOSOCKET )( "%s - i recieved %d/%d bytes from client \n", curr_local_time(), ret, bytes_to_receive_in_bufferforserver );
      w_offset_bufferforserver += ret;
      bytes_to_send_in_bufferforserver += ret;
      bytes_to_receive_in_bufferforserver -= ret;
    }
    else
    {
      if( ret < 0 )
      {
        if( ret == -2 )
        {
          TRACE( TRACE_CONNECTIONS )( "%s - gracefully client disconnetion, I could not recieve\n", curr_local_time() );
          closeClient();
          if( pjq && pinfo )
          {
            if( pServerConnected )
              pjq->post( STATUS_CLIENT_DISCONNECTED_OK, pintptr( pinfo ) );
            else
              pjq->post( STATUS_CONNECTION_FAILED, pintptr( pinfo ) );
          }
        }
        else
        {
          TRACE( TRACE_CONNECTIONS )( "%s - unexpected client disconnetion, I could not recieve\n", curr_local_time() );
          closeClient();
          if( pjq && pinfo )
          {
            if( pServerConnected )
              pjq->post( STATUS_CLIENT_CONNECTION_LOST, pintptr( pinfo ) );
            else
              pjq->post( STATUS_CONNECTION_FAILED, pintptr( pinfo ) );

          }
        }
      }
    }
  }
}

void ConnectionPeer::getMissingDataFromServer()
{
  int ret;

  if( pServer && bytes_to_receive_in_bufferforclient )
  {
    ret = pServer->recieveNB( &bufferforclient[w_offset_bufferforclient], bytes_to_receive_in_bufferforclient );
    if( ret > 0 )
    {
      TRACE( TRACE_IOSOCKET )( "%s - i read %d/%d bytes from server\n", curr_local_time(), ret, bytes_to_receive_in_bufferforclient );
      w_offset_bufferforclient += ret;
      bytes_to_receive_in_bufferforclient -= ret;
      bytes_to_send_in_bufferforclient += ret;
    }
    else
    {
      if( ret < 0 )
      {
        if( ret == -2 )
        {
          TRACE( TRACE_CONNECTIONS )( "%s - gracefully sever disconnetion, I could not recieve\n", curr_local_time() );
          closeServer();
          if( pjq && pinfo )
          {
            pjq->post( STATUS_SERVER_DISCONNECTED_OK, pintptr( pinfo ) );
          }
        }
        else
        {
          TRACE( TRACE_CONNECTIONS )( "%s - unexpected sever disconnetion, I could not recieve\n", curr_local_time() );
          closeServer();
          if( pjq && pinfo )
          {
            pjq->post( STATUS_SERVER_CONNECTION_LOST, pintptr( pinfo ) );
          }
        }
      }
    }
  }
}

void ConnectionPeer::sendMissingDataToClient()
{
  int ret;

  if( pClient && bytes_to_send_in_bufferforclient )
  {
    ret = pClient->sendNB( &bufferforclient[r_offset_bufferforclient], bytes_to_send_in_bufferforclient );
    if( ret > 0 )
    {
      TRACE( TRACE_IOSOCKET )( "%s - i sent %d/%d bytes from client\n", curr_local_time(), ret, bytes_to_send_in_bufferforclient );
      r_offset_bufferforclient += ret;
      bytes_to_send_in_bufferforclient -= ret;

      if( w_offset_bufferforclient && !bytes_to_send_in_bufferforclient )
      {
        r_offset_bufferforclient = 0;
        w_offset_bufferforclient = 0;
        bytes_to_receive_in_bufferforclient = STAMBUFFERSOCKET;
      }

      if( !pServer )
      {
        TRACE( TRACE_CONNECTIONS )( "%s - i sent to client but server was already disconnected\n", curr_local_time() );
        if( !bytes_to_send_in_bufferforclient )
        {
          closeClient();
          if( pjq && pinfo )
          {
            pjq->post( STATUS_PEER_CONNECTION_CANCELLED, pintptr( pinfo ) );
          }
        }
      }
    }
    else
    {
      if( ret < 0 )
      {
        if( ret == -2 )
        {
          TRACE( TRACE_CONNECTIONS )( "%s - gracefully client disconnetion, I could not send\n", curr_local_time() );
          closeClient();
          if( pjq && pinfo )
          {
            pjq->post( STATUS_CLIENT_DISCONNECTED_OK, pintptr( pinfo ) );
          }
        }
        else
        {
          TRACE( TRACE_CONNECTIONS )( "%s - unexpected client disconnetion, I could not sent\n", curr_local_time() );
          closeClient();
          if( pjq && pinfo )
          {
            pjq->post( STATUS_CLIENT_CONNECTION_LOST, pintptr( pinfo ) );
          }
        }
      }
    }
  }
}

void ConnectionPeer::sendMissingDataToServer()
{
  int ret;

  if( pServer && bytes_to_send_in_bufferforserver )
  {
    ret = pServer->sendNB( &bufferforserver[r_offset_bufferforserver], bytes_to_send_in_bufferforserver );
    if( ret > 0 )
    {
      TRACE( TRACE_IOSOCKET )( "%s - i sent %d/%d bytes to server\n", curr_local_time(), ret, bytes_to_send_in_bufferforserver );
      r_offset_bufferforserver += ret;
      bytes_to_send_in_bufferforserver -= ret;

      if( w_offset_bufferforserver && !bytes_to_send_in_bufferforserver )
      {
        w_offset_bufferforserver = 0;
        r_offset_bufferforserver = 0;
        bytes_to_receive_in_bufferforserver = STAMBUFFERSOCKET;
      }

      if( !pClient )
      {
        TRACE( TRACE_CONNECTIONS )( "%s - i sent to sever but client was already disconnected\n", curr_local_time() );
        if( !bytes_to_send_in_bufferforserver )
        {
          closeServer();
          if( pjq && pinfo )
          {
            pjq->post( STATUS_PEER_CONNECTION_CANCELLED, pintptr( pinfo ) );
          }
        }
      }
    }
    else
    {
      if( ret < 0 )
      {
        if( ret == -2 )
        {
          TRACE( TRACE_CONNECTIONS )( "%s - gracefully server disconnetion, I could not send\n", curr_local_time() );
          closeServer();
          if( pjq && pinfo )
          {
            pjq->post( STATUS_SERVER_DISCONNECTED_OK, pintptr( pinfo ) );
          }
        }
        else
        {
          TRACE( TRACE_CONNECTIONS )( "%s - unexpected server disconnetion, I could not send\n", curr_local_time() );
          closeServer();
          if( pjq && pinfo )
          {
            pjq->post( STATUS_SERVER_CONNECTION_LOST, pintptr( pinfo ) );
          }
        }
      }
    }
  }
}

bool ConnectionPeer::checkPeers()
{
  bool val = false;

  if( pServer )
  {
    if( pServer->checkSocket() < 0 )
    {
      TRACE( TRACE_IOSOCKET )( "%s - there was an error with server socket\n", curr_local_time() );
      TRACE( TRACE_CONNECTIONS )( "%s - unexpected server disconnetion, select() function failed\n", curr_local_time() );
      closeServer();
      if( pjq && pinfo )
      {
        pjq->post( STATUS_SERVER_CONNECTION_LOST, pintptr( pinfo ) );
      }
    }
  }

  if( pClient )
  {

    if( pClient->checkSocket() < 0 )
    {
      TRACE( TRACE_IOSOCKET )( "%s - There was an error with client socket\n", curr_local_time() );
      TRACE( TRACE_CONNECTIONS )( "%s - unexpected client disconnetion, select() function failed\n", curr_local_time() );
      closeClient();
      if( pjq && pinfo )
      {
        pjq->post( STATUS_CLIENT_CONNECTION_LOST, pintptr( pinfo ) );
      }
    }
  }

  return val;
}

bool ConnectionPeer::checkConnectingPeers( /*fd_set * setr*/ )
{
  bool val = false;

  if( pServer )
  {
    if( pServer->checkSocket() < 0 )
    {
      TRACE( TRACE_CONNECTIONS )( "%s - unexpected server disconnetion, select() function failed (2)\n", curr_local_time() );
      closeServer();
      if( pjq && pinfo )
      {
        pjq->post( STATUS_SERVER_CONNECTION_LOST, pintptr( pinfo ) );
      }
    }

  }

  if( pClient )
  {
    if( pClient->checkSocket() < 0 )
    {
      TRACE( TRACE_CONNECTIONS )( "%s - unexpected client disconnetion, select() function failed (2)\n", curr_local_time() );
      closeClient();
      if( pjq && pinfo )
      {
        pjq->post( STATUS_CLIENT_CONNECTION_LOST, pintptr( pinfo ) );
      }
    }
  }

  return val;
}
