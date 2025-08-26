/*
   ConnectionPeer.cpp: connection peer class source file.

   Copyright 2006, 2007, 2008, 2009 Juan Rey Saura

This file is part of RELB Easy Load Balancer.

This software is a copyrighted work licensed under the Open Software License version 3.0
Please consult the file "LICENSE.txt" for details.
*/

#include "ConnectionPeer.h"

#include <stdio.h>
#include "utiles.h"

//Testing option
//disable it if unsure
//#define OPTIMIZE_PEER_READING 

// Constructor: Initializes a ConnectionPeer with client/server sockets, message queue, and peer info
ConnectionPeer::ConnectionPeer( Socket * pcli, Socket * psrv, const MessageQueue * jq, const peer_info * info )
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
  pjq = (MessageQueue *) jq;
  pinfo = info;
  pServerConnected = false;
  TRACE( TRACE_CONNECTIONS )( "%s - ConnectionPeer created: client=%p, server=%p\n", curr_local_time(), (void *) pClient, (void *) pServer );
}

// Destructor: Closes and deletes client/server
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

/// <summary>
/// Adds the client and server sockets to the fd_set for reading if there is buffer space.
/// </summary>
void ConnectionPeer::addToFDSETR( fd_set * set )
{
  TRACE( TRACE_CONNECTIONS && TRACE_VERY_VERBOSE )( "%s - pClient=%p, %d bytes free in buffer\n", curr_local_time(), (void *) pClient, bytes_to_receive_in_bufferforserver );

  if( pClient && bytes_to_receive_in_bufferforserver )
  {
    pClient->addToFDSET( set );
    TRACE( TRACE_CONNECTIONS && TRACE_VERY_VERBOSE )( "%s - Added client to FDSETR\n", curr_local_time() );
  }

  TRACE( TRACE_CONNECTIONS && TRACE_VERY_VERBOSE )( "%s - pServer=%p, %d bytes free in buffer\n", curr_local_time(), (void *) pServer, bytes_to_receive_in_bufferforclient );

  if( pServer && bytes_to_receive_in_bufferforclient )
  {
    pServer->addToFDSET( set );
    TRACE( TRACE_CONNECTIONS && TRACE_VERY_VERBOSE )( "%s - Added server to FDSETR\n", curr_local_time() );
  }
}

/// <summary>
/// Adds the client and server sockets to the fd_set for writing if there is data to send.
/// </summary>
void ConnectionPeer::addToFDSETW( fd_set * set )
{
  if( pClient && bytes_to_send_in_bufferforclient )
  {
    pClient->addToFDSET( set );
    TRACE( TRACE_CONNECTIONS && TRACE_VERY_VERBOSE )( "%s - Added client to FDSETW\n", curr_local_time() );
  }

  if( pServer && bytes_to_send_in_bufferforserver )
  {
    pServer->addToFDSET( set );
    TRACE( TRACE_CONNECTIONS && TRACE_VERY_VERBOSE )( "%s - Added server to FDSETW\n", curr_local_time() );
  }
}

/// <summary>
/// Adds the client and server sockets to the fd_sets for checking connection status.
/// </summary>
void ConnectionPeer::addToFDSETC( fd_set * setr, fd_set * setw )
{

  if( pClient && bytes_to_receive_in_bufferforserver )
  {
    TRACE( TRACE_CONNECTIONS )( "%s - Checking ongoing connection A (client)\n", curr_local_time() );
    pClient->addToFDSET( setr );
  }

  if( pServer )
  {
    TRACE( TRACE_CONNECTIONS )( "%s - Checking ongoing connection B (server)\n", curr_local_time() );
    pServer->addToFDSET( setw );
  }
}

/// <summary>
/// If the given peer_info matches this peer, close the connection and notify via message queue.
/// </summary>
/// <param name="pcheck">The peer_info to check against the current peer's info.</param>
void ConnectionPeer::closePInfo( peer_info * pcheck )
{
  if( pcheck == pinfo )
  {
    TRACE( TRACE_CONNECTIONS && TRACE_VERY_VERBOSE )( "%s - Closing peer due to matching peer_info\n", curr_local_time() );
    close();
    if( pjq && pinfo )
    {
      pjq->post( STATUS_SERVER_DISCONNECTED_OK, pintptr( pinfo ) );
      TRACE( TRACE_CONNECTIONS && TRACE_VERY_VERBOSE )( "%s - Posted STATUS_SERVER_DISCONNECTED_OK\n", curr_local_time() );
    }
  }
}

// Close both client and server sockets
void ConnectionPeer::close()
{
  closeClient();
  closeServer();
}

// Close the client socket and reset related state
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

    // If server is present but has nothing to send, close it too
    if( pServer && !bytes_to_send_in_bufferforserver )
    {
      closeServer();
    }
    else
    {
      if( !pServerConnected )
      {
        closeServer();
      }
    }
  }

  if( !isActive() )
  {
    TRACE( TRACE_CONNECTIONS && TRACE_VERBOSE )( "%s - Client and server are already closed (Client)\n", curr_local_time() );
  }
}

#ifdef DEBUG
// Debug/test-only close methods
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

// Close the server socket and reset related state
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

    // If client is present but has nothing to send, close it too
    if( pClient && !bytes_to_send_in_bufferforclient )
    {
      closeClient();
    }
  }

  if( !isActive() )
  {
    TRACE( TRACE_CONNECTIONS && TRACE_VERBOSE )( "%s - Client and server are already closed (Server)\n", curr_local_time() );
  }
}

// Check if the connection peer is active, meaning either client or server is connected
bool ConnectionPeer::isActive()
{
  return ( ( pClient ) || ( pServer ) );
}

/// <summary>
/// Manage the connecting peer, according to the data available in both fd_set.
/// </summary>
/// <param name="setr"></param>
/// <param name="setw"></param>
/// <returns>True if the peer status changed, false otherwise.</returns>
bool ConnectionPeer::manageConnectingPeer( fd_set * setr, fd_set * setw )
{
  bool ret = false;

  if( pClient && pServer )
  {

    if( pClient->isInFDSET( setr ) )
    {
      // There is data to read from the client
      TRACE( TRACE_IOSOCKET )( "%s - reading from client during connect\n", curr_local_time() );
      getMissingDataFromClient();
    }

    if( pServer )
    {
      if( pServer->isInFDSET( setw ) )
      {
        // Server socket is ready to write, so connection is established
        TRACE( TRACE_IOSOCKET )( "%s - connecting to server (established)\n", curr_local_time() );
        pServerConnected = true;
        if( pjq && pinfo )
        {
          pjq->post( STATUS_CONNECTION_ESTABLISHED, pintptr( pinfo ) );
          TRACE( TRACE_CONNECTIONS )( "%s - Posted STATUS_CONNECTION_ESTABLISHED\n", curr_local_time() );
        }
        ret = true;
      }
    }
  }
  else
  {
    // One peer is disconnected; handle cleanup
    if( pClient )
    {
      if( pClient->isInFDSET( setw ) )
      {
        TRACE( TRACE_IOSOCKET )( "%s - able to write to client during connect\n", curr_local_time() );
        sendMissingDataToClient();
      }

      if( !bytes_to_send_in_bufferforclient )
      {
        TRACE( TRACE_CONNECTIONS )( "%s - killing client, nothing to be sent from server (connect)\n", curr_local_time() );
        closeClient();
        if( pjq && pinfo )
        {
          pjq->post( STATUS_PEER_CONNECTION_CANCELLED, pintptr( pinfo ) );
          TRACE( TRACE_CONNECTIONS )( "%s - Posted STATUS_PEER_CONNECTION_CANCELLED (client)\n", curr_local_time() );
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
      // client was already disconnected, so we can close the server
      TRACE( TRACE_CONNECTIONS && TRACE_VERY_VERBOSE )( "%s - killing server, nothing to be sent from client (connect)\n", curr_local_time() );
      closeServer();
      if( pjq && pinfo )
      {
        pjq->post( STATUS_PEER_CONNECTION_CANCELLED, pintptr( pinfo ) );
        TRACE( TRACE_CONNECTIONS && TRACE_VERY_VERBOSE )( "%s - Posted STATUS_PEER_CONNECTION_CANCELLED (server)\n", curr_local_time() );
      }
      ret = false;
    }
  }

  return ret;
}

/// <summary>
/// Manage the connections of the peer, according to the data available in both fd_set.
/// </summary>
void ConnectionPeer::manageConnections( fd_set * setr, fd_set * setw )
{

  if( pClient && pServer )
  {
    TRACE( TRACE_IOSOCKET && TRACE_VERY_VERBOSE )( "%s - Entering manageConnections\n", curr_local_time() );
    if( pClient->isInFDSET( setr ) )
    {
      // There is data to read from the client
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
      // Server socket is ready to write
      if( pServer->isInFDSET( setw ) )
      {
        TRACE( TRACE_IOSOCKET )( "%s - i can send to server\n", curr_local_time() );
        sendMissingDataToServer();
      }
#ifndef OPTIMIZE_PEER_READING 
    }
#endif

    if( pServer ) // is really necessary?
    {
      if( pServer->isInFDSET( setr ) )
      {
        // There is data to read from the server  
        TRACE( TRACE_IOSOCKET )( "%s - reading from server\n", curr_local_time() );
        getMissingDataFromServer();
#ifndef OPTIMIZE_PEER_READING 
        sendMissingDataToClient();
#endif
      }
#ifndef OPTIMIZE_PEER_READING 
      else
      {
#endif
        // Client socket is ready to write
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
    // Only one peer is connected; handle cleanup
    if( pClient )
    {
      if( pClient->isInFDSET( setw ) )
      {
        TRACE( TRACE_IOSOCKET )( "%s - i can send to client (single)\n", curr_local_time() );
        sendMissingDataToClient();
      }
      // if there is no data to send to the client, otherwise peer will be not been removed from the list in the ThreadedConnectionManager
      if( !bytes_to_send_in_bufferforclient )
      {
        TRACE( TRACE_CONNECTIONS )( "%s - killing client, nothing to be sent to server (single)\n", curr_local_time() );
        closeClient();
        if( pjq && pinfo )
        {
          pjq->post( STATUS_PEER_CONNECTION_CANCELLED, pintptr( pinfo ) );
          TRACE( TRACE_CONNECTIONS )( "%s - Posted STATUS_PEER_CONNECTION_CANCELLED (client single)\n", curr_local_time() );
        }
      }
    }
    if( pServer )
    {
      if( pServer->isInFDSET( setw ) )
      {
        TRACE( TRACE_IOSOCKET )( "%s - i can send to server (single)\n", curr_local_time() );
        sendMissingDataToServer();
      }

      if( !bytes_to_send_in_bufferforserver )
      {
        TRACE( TRACE_CONNECTIONS )( "%s - killing server, nothing else to send to client (single)\n", curr_local_time() );
        closeServer();
        if( pjq && pinfo )
        {
          pjq->post( STATUS_PEER_CONNECTION_CANCELLED, pintptr( pinfo ) );
          TRACE( TRACE_CONNECTIONS )( "%s - Posted STATUS_PEER_CONNECTION_CANCELLED (server single)\n", curr_local_time() );
        }
      }
    }
  }
}

// Read data from client and update buffer state
void ConnectionPeer::getMissingDataFromClient()
{
  int ret;

  if( pClient && bytes_to_receive_in_bufferforserver )
  {
    ret = pClient->receiveNB( &bufferforserver[w_offset_bufferforserver], bytes_to_receive_in_bufferforserver );
    if( ret > 0 )
    {
      TRACE( TRACE_IOSOCKET )( "%s - received %d/%d bytes from client\n", curr_local_time(), ret, bytes_to_receive_in_bufferforserver );
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
          TRACE( TRACE_CONNECTIONS )( "%s - graceful client disconnection, could not receive\n", curr_local_time() );
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
          TRACE( TRACE_CONNECTIONS )( "%s - unexpected client disconnection, could not receive\n", curr_local_time() );
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

// Read data from server and update buffer state
void ConnectionPeer::getMissingDataFromServer()
{
  int ret;

  if( pServer && bytes_to_receive_in_bufferforclient )
  {
    ret = pServer->receiveNB( &bufferforclient[w_offset_bufferforclient], bytes_to_receive_in_bufferforclient );
    if( ret > 0 )
    {
      TRACE( TRACE_IOSOCKET )( "%s - read %d/%d bytes from server\n", curr_local_time(), ret, bytes_to_receive_in_bufferforclient );
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
          TRACE( TRACE_CONNECTIONS )( "%s - graceful server disconnection, could not receive\n", curr_local_time() );
          closeServer();
          if( pjq && pinfo )
          {
            pjq->post( STATUS_SERVER_DISCONNECTED_OK, pintptr( pinfo ) );
          }
        }
        else
        {
          TRACE( TRACE_CONNECTIONS )( "%s - unexpected server disconnection, could not receive\n", curr_local_time() );
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

// Send data to client and update buffer state               
void ConnectionPeer::sendMissingDataToClient()
{
  int ret;

  if( pClient && bytes_to_send_in_bufferforclient )
  {
    ret = pClient->sendNB( &bufferforclient[r_offset_bufferforclient], bytes_to_send_in_bufferforclient );
    if( ret > 0 )
    {
      TRACE( TRACE_IOSOCKET )( "%s - sent %d/%d bytes to client\n", curr_local_time(), ret, bytes_to_send_in_bufferforclient );
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
        TRACE( TRACE_CONNECTIONS )( "%s - sent to client but server already disconnected\n", curr_local_time() );
        if( !bytes_to_send_in_bufferforclient )
        {
          closeClient();
          if( pjq && pinfo )
          {
            pjq->post( STATUS_PEER_CONNECTION_CANCELLED, pintptr( pinfo ) );
            TRACE( TRACE_CONNECTIONS )( "%s - Posted STATUS_PEER_CONNECTION_CANCELLED (client, server gone)\n", curr_local_time() );
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
          TRACE( TRACE_CONNECTIONS )( "%s - graceful client disconnection, could not send\n", curr_local_time() );
          closeClient();
          if( pjq && pinfo )
          {
            pjq->post( STATUS_CLIENT_DISCONNECTED_OK, pintptr( pinfo ) );
          }
        }
        else
        {
          TRACE( TRACE_CONNECTIONS )( "%s - unexpected client disconnection, could not send\n", curr_local_time() );
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

// Send data to server and update buffer state
void ConnectionPeer::sendMissingDataToServer()
{
  int ret;

  if( pServer && bytes_to_send_in_bufferforserver )
  {
    ret = pServer->sendNB( &bufferforserver[r_offset_bufferforserver], bytes_to_send_in_bufferforserver );
    if( ret > 0 )
    {
      TRACE( TRACE_IOSOCKET )( "%s - sent %d/%d bytes to server\n", curr_local_time(), ret, bytes_to_send_in_bufferforserver );
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
        TRACE( TRACE_CONNECTIONS )( "%s - sent to server but client already disconnected\n", curr_local_time() );
        if( !bytes_to_send_in_bufferforserver )
        {
          closeServer();
          if( pjq && pinfo )
          {
            pjq->post( STATUS_PEER_CONNECTION_CANCELLED, pintptr( pinfo ) );
            TRACE( TRACE_CONNECTIONS )( "%s - Posted STATUS_PEER_CONNECTION_CANCELLED (server, client gone)\n", curr_local_time() );
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
          TRACE( TRACE_CONNECTIONS )( "%s - graceful server disconnection, could not send\n", curr_local_time() );
          closeServer();
          if( pjq && pinfo )
          {
            pjq->post( STATUS_SERVER_DISCONNECTED_OK, pintptr( pinfo ) );
          }
        }
        else
        {
          TRACE( TRACE_CONNECTIONS )( "%s - unexpected server disconnection, could not send\n", curr_local_time() );
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

// Check both server and client sockets for errors using select()
bool ConnectionPeer::checkPeers()
{
  bool val = false;

  if( pServer )
  {
    if( pServer->checkSocket() < 0 )
    {
      TRACE( TRACE_IOSOCKET )( "%s - error with server socket\n", curr_local_time() );
      TRACE( TRACE_CONNECTIONS )( "%s - unexpected server disconnection, select() failed\n", curr_local_time() );
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
      TRACE( TRACE_IOSOCKET )( "%s - error with client socket\n", curr_local_time() );
      TRACE( TRACE_CONNECTIONS )( "%s - unexpected client disconnection, select() failed\n", curr_local_time() );
      closeClient();
      if( pjq && pinfo )
      {
        pjq->post( STATUS_CLIENT_CONNECTION_LOST, pintptr( pinfo ) );
      }
    }
  }

  return val;
}

// Check both server and client sockets for errors during connection establishment
bool ConnectionPeer::checkConnectingPeers( /*fd_set * setr*/ )
{
  bool val = false;

  if( pServer )
  {
    if( pServer->checkSocket() < 0 )
    {
      TRACE( TRACE_CONNECTIONS )( "%s - unexpected server disconnection, select() failed (connect)\n", curr_local_time() );
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
      TRACE( TRACE_CONNECTIONS )( "%s - unexpected client disconnection, select() failed (connect)\n", curr_local_time() );
      closeClient();
      if( pjq && pinfo )
      {
        pjq->post( STATUS_CLIENT_CONNECTION_LOST, pintptr( pinfo ) );
      }
    }
  }

  return val;
}
