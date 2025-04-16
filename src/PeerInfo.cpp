/*
   PeerInfo.cpp: peer_info class source file.

   Copyright 2006, 2007, 2008, 2009 Juan Rey Saura

This file is part of Resolutive Easy Load Balancer.

This software is a copyrighted work licensed under the Open Software License version 3.0
Please consult the file "LICENSE.txt" for details.
*/

#include "PeerInfo.h"

peer_info::peer_info()
{
}

peer_info::~peer_info()
{
}

void peer_info::setParallel( peer_info * p )
{
  p->copy( this );
  p->parallel = this;
  this->parallel = p;
}

void peer_info::copy( peer_info * p )
{
  src_ip = p->src_ip;
  src_port = p->src_port;
  dst_ip = p->dst_ip;
  dst_port = p->dst_port;
  status = p->status;
  parallel = p->parallel;
  manager = p->manager;
  created = p->created;
  modified = p->modified;
  last_changed = p->last_changed;
  ban_this = p->ban_this;
  ban_all = p->ban_all;
}
