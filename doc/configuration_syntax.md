# Relb Configuration File Syntax

This document explains the syntax and options for the `relb.conf` configuration file.

## General Syntax

The configuration file is a plain text file. 
- Comment lines can start with `#`, `;`, or `/`.
- Each configuration option is defined by a tag followed by a colon and its value(s).

---

## Configuration Options

### `bind`

- **Description:** Specifies the address and port where the application will listen for incoming connections. This is a mandatory option.
- **Format:** `bind: [[server_name | IPv4_address]:]port`
- **Example:** 
  ```
  bind: 0.0.0.0:4389
  ```
  This example makes the application listen on all available network interfaces on port 4389.

### `alsobind`

- **Description:** Allows you to specify additional addresses and ports to listen on, in addition to the primary `bind` address. This is an optional, multi-entry tag.
- **Format:** `alsobind: [[server_name | IPv4_address]:]port`
- **Example:**
  ```
  alsobind: 127.0.0.1:4390
  ```

### `server`

- **Description:** Defines the backend servers that will handle the incoming connections. You can have multiple `server` entries. This is a mandatory option.
- **Format:** `server: (server_name | IPv4_address):port[,weight[,max_connections]]`
  - `weight` (optional): A numerical value that determines the load balancing weight. A higher value means more traffic will be directed to that server.
  - `max_connections` (optional): The maximum number of simultaneous connections that can be made to this server.
- **Example:**
  ```
  server: 192.168.1.201:3389,100,20
  server: 192.168.1.202:3389,120
  ```

### `retryserver`

- **Description:** Sets the time in seconds to wait before retrying a connection to a server that was previously unavailable.
- **Format:** `retryserver: seconds`
- **Default:** `30`
- **Example:**
  ```
  retryserver: 60
  ```

### `filter`

- **Description:** Allows you to define rules to filter incoming connections based on the client and server IP addresses. The rules are processed in the order they appear in the file.
- **Format:** `filter: client_ip/mask server_ip/mask "allow"|"deny"`
  - `mask`: Can be in dotted decimal notation (e.g., `255.255.255.0`) or as a CIDR number (e.g., `24`).
- **Example:**
  ```
  filter: 192.168.102.1/255.255.255.0 192.168.1.201/255.255.255.255 allow
  ```

### `admin`

- **Description:** Enables a web-based administration interface on a specified port. **Warning:** This interface is unencrypted and unauthenticated, so use it with caution.
- **Format:** `admin: [[server_name | IPv4_address]:]port`
- **Default:** `127.0.0.1:8182`
- **Example:**
  ```
  admin: 0.0.0.0:8182
  ```

### `task`

- **Description:** Configures automated tasks for server management. This is an optional, multi-entry tag.
- **Format:** `task: task_type interval_seconds [first_run_datetime]`
  - `task_type`:
	- `update`: Updates server IPs with hostnames (not yet implemented).
	- `clean`: Cleans inactive connections.
	- `purge`: Closes all connections to servers.
  - `interval_seconds`: The interval in seconds between task executions. Use `0` for a one-time task.
  - `first_run_datetime` (optional): The date and/or time to run the task for the first time.
	- **Date Format:** `YYYYMMDD` or `YYYY/MM/DD`
	- **Time Format:** `HH:mm:ss` (24-hour)
- **Example:**
  ```
  task: clean 3600 08:00:00
  ```

### `conperthread`

- **Description:** An advanced option to configure the number of connections per thread for internal load balancing.
- **Format:** `conperthread: num_connections`
- **Default:** `16`
- **Warning:** The maximum number of connections is limited by `FD_SETSIZE` (typically 1024 on Linux and 256 on Windows).
- **Example:**
  ```
  conperthread: 32
  ```
