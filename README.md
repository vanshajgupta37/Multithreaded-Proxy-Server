# Multithreaded-Proxy-Server

This project implements a multithreaded proxy server in C with caching capabilities. It acts as an intermediary between clients and web servers, forwarding HTTP requests and responses, while also caching responses for improved performance.

## Features

- Multithreaded handling of client connections
- Support for HTTP proxying
- Caching of server responses
- LRU (Least Recently Used) cache eviction policy
- Configurable listening port
- Error handling and logging



## Prerequisites

To compile and run this project, you need:

- A C compiler (e.g., gcc)
- POSIX-compliant operating system (Linux, macOS, etc.)
- pthread library


