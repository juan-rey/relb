/* 
   ConnectionPeerList.h: connection peer list class header file.

   Copyright 2006, 2007, 2008, 2009 Juan Rey Saura

This file is part of Resolutive Easy Load Balancer.

This software is a copyrighted work licensed under the Open Software License version 3.0
Please consult the file "LICENSE.txt" for details.
*/

#ifndef ConnectionPeerList_h
#define ConnectionPeerList_h

#include <pasync.h>

USING_PTYPES

#include "ConnectionPeer.h"

class ConnectionPeerList 
{
public:
    tpodlist<ConnectionPeer> lista;
};
#endif

