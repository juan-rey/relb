/*
   PeerInfo.h: peer_info class header file.

   Copyright 2006, 2007, 2008, 2009 Juan Rey Saura

This file is part of RELB Easy Load Balancer.

This software is a copyrighted work licensed under the Open Software License version 3.0
Please consult the file "LICENSE.txt" for details.
*/

#ifndef PeerInfo_h
#define PeerInfo_h

#include <pasync.h>
#include <pinet.h>
#include <ptime.h>

USING_PTYPES

class peer_info
{
public:
  peer_info();
  virtual ~peer_info();
  void setParallel( peer_info * p );
public:
  void copy( peer_info * p );//   { ldata = a.ldata; return *this; }    
  ipaddress src_ip;
  unsigned short src_port;
  ipaddress dst_ip;
  unsigned short dst_port;
  unsigned char status;
  peer_info * parallel;
  void * manager;
  datetime created;
  datetime modified;
  datetime last_changed;
  bool ban_this;
  bool ban_all;
};

#endif
