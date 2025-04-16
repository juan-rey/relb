/*
   ConnectionPeer.h: connection peer class header file.

   Copyright 2006, 2007, 2008, 2009 Juan Rey Saura

This file is part of Resolutive Easy Load Balancer.

This software is a copyrighted work licensed under the Open Software License version 3.0
Please consult the file "LICENSE.txt" for details.
*/

#ifndef ConnectionPeer_h
#define ConnectionPeer_h

#include "Socket.h"
#include "utiles.h"
#include "PeerInfo.h"
#include "MessaggeQueue.h"

#define STAMBUFFERSOCKET 16384

class ConnectionPeer {

public:
  ConnectionPeer( Socket * pcli, Socket * psrv, const MessaggeQueue * jq, const peer_info * info );
  ~ConnectionPeer();

  void close();
  void manageConnections( fd_set * setr, fd_set * setw );
  bool manageConnectingPeer( fd_set * setr, fd_set * setw );
  void addToFDSETR( fd_set * set );
  void addToFDSETW( fd_set * set );
  void addToFDSETC( fd_set * setr, fd_set * setw );
  bool isActive();
  bool checkPeers();
  bool checkConnectingPeers();
  void closePInfo( peer_info * pcheck );

private:
  void getMissingDataFromServer();
  void getMissingDataFromClient();
  void sendMissingDataToClient();
  void sendMissingDataToServer();
  char bufferforserver[STAMBUFFERSOCKET];
  int w_offset_bufferforserver;
  int r_offset_bufferforserver;
  int bytes_to_receive_in_bufferforserver;
  int bytes_to_send_in_bufferforserver;
  char bufferforclient[STAMBUFFERSOCKET];
  int w_offset_bufferforclient;
  int r_offset_bufferforclient;
  int bytes_to_receive_in_bufferforclient;
  int bytes_to_send_in_bufferforclient;
  MessaggeQueue * pjq;
  const peer_info * pinfo;
  bool  pServerConnected;
#ifdef DEBUG
public:
#endif
  void closeClient();
  void closeServer();
#ifdef DEBUG
  void closeServer2();
  void closeClient2();
  void closeServer3();
  void closeClient3();
#endif

public:
  Socket * pServer;
  Socket * pClient;
};
#endif
