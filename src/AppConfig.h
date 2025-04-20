/*
   AppConfig.h: config class header file.

   Copyright 2006, 2007, 2008, 2009, 2025 Juan Rey Saura

This file is part of Resolutive Easy Load Balancer.

This software is a copyrighted work licensed under the Open Software License version 3.0
Please consult the file "LICENSE.txt" for details.
*/

#ifndef AppConfig_h
#define AppConfig_h

#include <pasync.h>
#include <pinet.h>

USING_PTYPES

#include "utiles.h"

#define DEFAULT_CONFIG_FILENAME "relb.conf"

#ifdef WIN32
#define DEFAULT_CONFIG_FILE_PATH "\\system32\\drivers\\etc\\"
#else
#define DEFAULT_CONFIG_FILE_PATH "/etc/"
#endif

struct dst_conf
{
  string servername;
  ipaddress dst_ip;
  unsigned short dst_port;
  int weight;
  int max_connections;
};

struct bind_conf
{
  tpodlist<bind_address *> address;
  tpodlist<dst_conf *> dst;
  tpodlist<task_info *> tasks;
  tpodlist<filterinfo *> filter;
};

class  AppConfig
{
public:
  AppConfig();
  virtual ~AppConfig();
  void setFilePath( const char * file );
  bool loadConfig();

private:
  bool setDefaultValues();
  bool loadFile();
  void processConfigLine( const char * line );
  bool checkConfig();
  void cleanBindList();
  void printErrors();
  void cleanErrors();

public:
  //config retrieving functions
  bool getFirstBind();
  bool getNextBind();
  /*
      unsigned short getBindPort();
      ipaddress getBindIP();
  */
  bool getFirstServer();
  bool getNextServer();
  unsigned short getServerPort();
  ipaddress getServerIP();
  string getServerName();
  int getServerWeight();
  int getServerMaxConnections();
  int getServerRetryTime();
  int getTasksCount();
  bool getNextTask();
  TASK_TYPE getTaskType();
  int getTaskInterval();
  datetime getTaskFirstRun();
  bool  getTaskFixedTime();
  bool getAdminEnabled();
  unsigned short getAdminPort();
  ipaddress getAdminIP();
  int getConnectionsPerThread();
  int getFilterCount();
  bool getNextFilter();
  ipaddress getFilterSourceIP();
  ipaddress getFilterSourceMask();
  ipaddress getFilterDestIP();
  ipaddress getFilterDestMask();
  bool getFilterAllow();
  int getAddressCount();
  bool getNextAddress();
  ipaddress getAddressIP();
  unsigned short getAddressPort();

private:
  //config variables
  string configfile;
  bool connect_to_the_same_server;
  bool one_connection_per_IP;
  bool http_admin_enabled;
  unsigned short http_admin_port;
  ipaddress http_admin_ip;
  int connections_per_thread;
  bool update_server_status;
  int minimun_server_reconnection_time;
  tpodlist<bind_conf *> bind;
  tpodlist<string *> errores;

  //auxiliar variables set to return servers values
  int currentBind;
  int currentServer;
  int currentTask;
  int currentFilter;
  int currentAddress;
};
#endif
