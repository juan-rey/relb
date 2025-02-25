# relb
RELB (RELB Easy Load Balancer)
=======================================

1. Introduction

RELB is a generic TCP load balancer. Its main features are: 

- Handles several hundred connections while using minimal system resources.
- Transparent to both clients and servers.
- Easy to configure.
- Flexible server assignment with smart routing or simple round-robin.
- Persistent connections with an IP address-based "Session Directory", ensuring that if a client loses connection, it is reassigned to the same server upon reconnection.
- Filters to allow or deny a client from being assigned to a server.
- Manageable via a built-in web admin page.
- Basic tasks, such as purging dead connections or disconnecting all connections.
- Multi-operating system and cross-platform support (Linux/Windows, etc.).

It is suitable for most TCP services/applications and can also be used as a port redirector.

2. Configuration

You must set up a proper configuration file in order for it to work. See relb.conf.sample for more information about the available options you can use.

3. Management

Use the web administration at [http://localhost:8182](http://localhost:8182) by default for basic management.

4. Acknowledgment

RELB uses Hovik Melikyan's C++ Portable Types Library (PTypes) (http://www.melikyan.com/ptypes/)

5. Support RELB
