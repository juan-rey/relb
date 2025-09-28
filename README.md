# RELB Easy Load Balancer

RELB (RELB Easy Load Balancer) is a tiny, cross-platform TCP load balancer focused on performance, reliability, flexibility, and simplicity. It works with most TCP services and applications and can also act as a port redirector.

Basicly, RELB listens for incoming TCP connections on specified IP addresses and ports, then forwards these connections to a pool of backend servers based on configurable rules and algorithms.

## Features

- Efficiently handles hundreds of simultaneous connections using minimal system resources.
- Operates transparently for both clients and servers.
- Plain-text configuration file that is simple and flexible.
- Maintains persistence using an IP-based "Session Directory" so clients reconnect to the same server.
- Intelligent server selection supporting weights, various assignment parameters, and round-robin behavior.
- Rule-based filtering system to allow or deny server assignment based on client/server IP rules.
- Integrated (optional) web administration interface for real-time monitoring and control.
- Scheduled maintenance tasks (e.g., purging inactive connections, disconnecting clients).
- Multi-platform: runs on Linux, Windows, and other major operating systems.

## Getting Started

1. Installation
   - See `INSTALL` for platform-specific build and install instructions.
2. Configuration
   - Copy or create a configuration file (see `relb.conf.sample` for all options and examples).
   - Place it in the default location or point to it with the command-line option.
3. Running RELB
   - Run `relb` from the command line. Optionally pass a configuration path.
   - Example: `relb config /etc/relb.conf`
4. Management
   - Access the web admin interface at http://localhost:8182 (default) if enabled.
   - Warning: The admin interface is unencrypted and unauthenticated; restrict access appropriately.

## Configuration

The configuration defines bind addresses, backend servers, filters, tasks, and other behavior. See `relb.conf.sample` and any additional documentation under `doc/` for details.

## Acknowledgments

RELB uses Hovik Melikyan's C++ Portable Types Library (PTypes) originally from [http://www.melikyan.com/](http://www.melikyan.com/). Slightly patched version for compilation with modern C++ compilers can be found at https://github.com/juan-rey/ptypes

## Limitations

- On Linux, the number of connections is limited by `FD_SETSIZE` (commonly 1024) and system resources.
- On Windows, limits are mainly the available ephemeral ports (49152–65535) and system resources.

## License

Licensed under the Open Software License version 3.0. See `LICENSE.txt`.

## Support

Report issues, request features, or contribute (see `doc/source_code_explained.md` for code structure) via the project repository: https://github.com/juan-rey/relb
