# RELB Easy Load Balancer

RELB (RELB Easy Load Balancer) is a high-performance, cross-platform TCP load balancer designed for reliability, flexibility, and ease of use. It is suitable for most TCP services and applications and can also function as a port redirector.

## Features

- Efficiently manages hundreds of simultaneous connections with minimal system resource usage.
- Operates transparently for both clients and servers.
- Simple and flexible configuration using a plain-text file.
- Maintains persistent connections via an IP address-based "Session Directory"; clients reconnect to the same server after disconnection.
- Intelligent server assignment supporting multiple variables and round-robin strategies.
- Advanced filtering system to allow or deny client-server assignments based on IP rules.
- Integrated web administration interface for real-time management and monitoring.
- Automated tasks, such as purging inactive connections or disconnecting all clients, can be scheduled.
- Multi-platform support: runs on Linux, Windows, and other major operating systems.

## Getting Started

1. **Installation**
   - Refer to the `INSTALL` file for platform-specific installation instructions.

2. **Configuration**
   - Create or edit your configuration file (see `relb.conf.sample` for all available options and examples).
   - Place the configuration file in the default location or specify it using the command-line option.

3. **Running RELB**
   - Start RELB from the command line. You can specify the configuration file path if needed.
   - Example: `relb -c /etc/relb.conf`

4. **Management**
   - Access the web administration interface at [http://localhost:8182](http://localhost:8182) (default).
   - Note: The admin interface is unencrypted and unauthenticated. Use with caution and restrict access as needed.

## Configuration

RELB uses a plain-text configuration file to define bind addresses, backend servers, filters, tasks, and more. See `doc\\configuration_syntax.md` and `relb.conf.sample` for detailed documentation and examples of each option.

## Acknowledgments

RELB uses Hovik Melikyan's C++ Portable Types Library (PTypes): [http://www.melikyan.com/ptypes/](http://www.melikyan.com/ptypes/)

## Limitations

The current implementation on Linux is limited by FD_SETSIZE (1024 file descriptors). With minor modifications, this limitation can be overcome, after which the limit will be determined by the maximum number of file descriptors per process (`ulimit -n`) and available system resources.

On Windows, there is no such file descriptors limitation. The primary limits are the available ephemeral socket ports (49152–65535; 16K) and system resources.

## License

RELB is licensed under the Open Software License version 3.0. See `LICENSE.txt` for details.

## Support

For questions, issues, or contributions, please refer to the project repository or contact the maintainer.
