
# RELB Easy Load Balancer Source Code Documentation

## 1. Overview

RELB (REsponsive Load Balancer) is a lightweight, easy-to-use load balancer written in C++. It is designed to distribute TCP connections among a group of servers. It can be configured to run as a Windows service or a Linux daemon.

## 2. Project Structure

The project is organized as follows:

```
.
├── src/
│   ├── main.cpp
│   ├── AppConfig.h
│   ├── AppConfig.cpp
│   ├── Bind.h
│   ├── Bind.cpp
│   ├── ConnectionPeer.h
│   ├── ConnectionPeer.cpp
│   ├── ConnectionPeerList.h
│   ├── ConnectionPeerList.cpp
│   ├── ThreadedConnectionManager.h
│   ├── ThreadedConnectionManager.cpp
│   ├── Socket.h
│   ├── Socket.cpp
│   ├── ServerList.h
│   ├── ServerList.cpp
│   ├── PeerInfo.h
│   ├── PeerInfo.cpp
│   ├── MessageQueue.h
│   ├── MessageQueue.cpp
│   ├── ControlSocket.h
│   ├── ControlSocket.cpp
│   ├── AdminHTTPServer.h
│   ├── utiles.h
│   ├── utiles.cpp
│   ├── win32service.h
│   ├── win32service.cpp
│   └── ...
├── configure.ac
├── Makefile.am
└── ...
```

## 3. Main Classes

### 3.1. `AppConfig`

*   **Header:** `src/AppConfig.h`
*   **Implementation:** `src/AppConfig.cpp`

The `AppConfig` class is responsible for parsing and managing the application's configuration. It reads the configuration from a file (e.g., `relb.conf`) and provides methods to access the configured parameters.

**Behavior:**

*   Loads the configuration file.
*   Parses settings for binds, servers, tasks, and filters.
*   Provides getters to access configuration values.

### 3.2. `Bind`

*   **Header:** `src/Bind.h`
*   **Implementation:** `src/Bind.cpp`

The `Bind` class is the core of the load balancer. It manages the listening sockets, incoming connections, and the distribution of connections to the backend servers.

**Behavior:**

*   Creates and manages listening sockets on specified IP addresses and ports.
*   Accepts incoming client connections.
*   Uses a `ThreadedConnectionManager` to handle connections.
*   Distributes connections to backend servers based on the configured load balancing algorithm (e.g., round-robin, weighted round-robin).
*   Manages a list of backend servers (`ServerList`).

### 3.3. `ThreadedConnectionManager`

*   **Header:** `src/ThreadedConnectionManager.h`
*   **Implementation:** `src/ThreadedConnectionManager.cpp`

This class manages a pool of threads to handle client connections. Each `ThreadedConnectionManager` is responsible for a set of connections, improving performance and scalability.

**Behavior:**

*   Creates and manages a pool of worker threads.
*   Assigns new connections to worker threads.
*   Uses `ConnectionPeer` objects to represent the connection between a client and a backend server.

### 3.4. `ConnectionPeer`

*   **Header:** `src/ConnectionPeer.h`
*   **Implementation:** `src/ConnectionPeer.cpp`

A `ConnectionPeer` object represents a pair of connected sockets: one for the client and one for the backend server. It is responsible for forwarding data between the two.

**Behavior:**

*   Reads data from the client socket and writes it to the server socket.
*   Reads data from the server socket and writes it to the client socket.
*   Manages the lifecycle of the connection.

### 3.5. `ServerList`

*   **Header:** `src/ServerList.h`
*   **Implementation:** `src/ServerList.cpp`

This class maintains lists of backend servers and client connections, automatically distributing incoming connections to the most appropriate available server.

**Behavior:**

*    Maintains a weighted list of backend servers, tracks their connection counts (connecting, connected, finished, disconnected), monitors their availability, and handles connection failures with retry timeouts to prevent overloading failed servers.
*    Intelligent Load Balancing: It implements a sophisticated server selection algorithm that considers server weights, current load ratios, connection limits, IP filtering rules, and session persistence (reconnecting clients to their previous servers when possible) to distribute incoming connections optimally.
*    Runs as a separate thread processing status messages from connections (coordinating with both the admin interface and connection managers to maintain real-time system state) and executing scheduled maintenance tasks  

### 3.6. `AdminHTTPServer`

*   **Header:** `src/AdminHTTPServer.h`
*   **Implementation:** `src/AdminHTTPServer.cpp`

The `AdminHTTPServer` class is a web-based management interface for the load balancer that provides real-time monitoring and administrative control capabilities.

**Behavior:**

* 	 Maintains real-time lists of active network connections (peers) and backend servers, tracking their status, connection counts, and timestamps through a message queue system that receives status updates.
*    Runs an HTTP server as a separate thread that serves HTML pages displaying connection lists, server status, and statistics. Also provides actions to manage connections including kicking individual connections, banning specific client IPs from connecting to particular servers, or banning all connections from a client IP across all servers.

## 4. Resource Usage

### 4.1. CPU

The CPU usage of RELB is primarily dependent on the number of active connections and the traffic volume. The use of a multi-threaded architecture (`ThreadedConnectionManager`) allows RELB to distribute the workload across multiple CPU cores, improving performance under high load.

### 4.2. Memory

Memory usage is influenced by the number of concurrent connections. Each `ConnectionPeer` object consumes memory to store socket information and data buffers. The memory usage will scale linearly with the number of active connections.

### 4.3. Threads

RELB uses a multi-threaded model to handle connections. A main thread is responsible for accepting new connections, and a pool of worker threads, managed by `ThreadedConnectionManager`, handles the data transfer for established connections. The number of worker threads is configurable, allowing for tuning based on the expected load and available system resources.
