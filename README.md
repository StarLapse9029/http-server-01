# Simple C HTTP Server

A small HTTP/1.1 server written in C using POSIX sockets.

A while back, I had the brilliant idea of making an HTTP server in assembly. I actually started working on it, reading through many man pages and the x86_64 ABI documentation. I implemented the TCP lifecycle—creating a socket, binding, listening, and accepting connections—as well as concurrency using `fork()`.

However, once I got to parsing HTTP requests, things started getting too complicated for me at the time, so I decided to switch to C.

## What I Learned

This project gave me a basic practical understanding of raw HTTP requests, the TCP lifecycle, POSIX sockets, and communication through file descriptors.

It also gave me experience with process-based concurrency using `fork()` and child-process management.

Finally, the project introduced me to several important Unix concepts, including:

* Signal handling
* Graceful shutdown
* `errno`-based error handling
* Network byte order
* Interrupted system calls (`EINTR`)
* Partial writes
* File descriptors and filesystem I/O

Overall, the project helped me connect concepts from operating systems, networking, HTTP, and filesystem I/O into a single working application.

## Features

* TCP server using POSIX sockets
* Listens on port `8080`
* Accepts multiple client connections
* Creates a child process for each connection using `fork()`
* Handles basic HTTP `GET` requests
* Serves static files from the current directory
* Returns `404 Not Found` when a requested file does not exist
* Graceful shutdown with `SIGINT` (`Ctrl+C`)
* Automatic child-process cleanup using `SIGCHLD`

## Requirements

* POSIX-compatible operating system
* C compiler
* POSIX networking and system libraries

This project was compiled and tested with GCC.

## Compilation

```bash
gcc -Wall -Wextra -o main main.c
```

## Running

Start the server with:

```bash
./main
```

The server listens on:

```text
http://localhost:8080
```

You can test it using a web browser or `curl`:

```bash
curl http://localhost:8080/
```

You can also request static files from the current directory:

```bash
curl http://localhost:8080/index.html
```

## TODO

* [ ] Implement protection against path traversal
* [ ] Improve HTTP request parsing
* [ ] Improve `EINTR` handling
* [ ] Handle `SIGPIPE`
* [ ] Improve error handling
* [ ] Add `Content-Length` headers
* [ ] Support appropriate MIME/content types
* [ ] Improve HTTP response handling

