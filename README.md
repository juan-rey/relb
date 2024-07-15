# relb
RELB (RELB Easy Load Balancer)
=======================================

1. Introduction

RELB is generic TCP load balancer, its main features are: 

- Handles several hundred connections with a low resources use.
- Transparent for client/server.
- Easily configurable.
- Flexible smart server assignation or just round-robin.
- Persisten connection, IP address based "Session Directory", so if client lost connection when it reconnects is assigned to the same server.
- Filters. Allow or deny a client to be assigned to a server.
- Manageable with a built-in web admin page.
- Basic tasks. Purge dead connections, disconnect all...
- Multi Operating Sysem and multiplataform (Linux/Windows...).

It is suitable for most of TCP services/applications and can also be used as a port redirector.

2. Configuration



3. Management

Use the web administration, http://localhost:8182 by default, for a basic management.

4. Acknowledgment

RELB uses Hovik Melikyan's C++ Portable Types Library (PTypes) (http://www.melikyan.com/ptypes/)

5. Support RELB


