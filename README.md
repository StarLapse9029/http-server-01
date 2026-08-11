# Simple C HTTP Server
Small HTTP/1.1 server written in C with POSIX sockets.  
A while back, I had the brilliant idea of making an HTTP server in assembly. 
I actually started working on it, reading through many man pages and the 
x86_64 ABI documentation. I even implemented the TCP lifecycle (creating a socket,
binding, listening, and accepting connections) as well as concurrency with fork().
However, once I got to parsing text, things started getting too complicated for
me at the time, so I decided to switch to C.

## What I Learned
This project gave me a basic pratical understanding of raw HTTP requests,
 TCP lifecycle, POSIX sockets and communication through file descriptors.  
It also gave me some experience with process based concurrency with fork() and
child process management.   
Finally, the project introduced several important Unix concepts, including
signal handling, graceful shutdown, errno-based error handling,
network byte order, interrupted system calls (EINTR), and partial writes.
Overall, the project helped connect concepts from operating systems,
networking, HTTP, and filesystem I/O into a single working application.

## Features
- TCP server using POSIX sockets
- Listens on port 8080
- Accepts multiple client connections
- Creates a child process for each connection using fork()
- Handles basic HTTP GET requests
- Serves static files from the current directory
- Returns 404 Not Found when a requested file does not exist
- Graceful shutdown with SIGINT (Ctrl+C)
- Automatic child-process cleanup using SIGCHLD

### Requirements
- POSIX compatible system and networking/system libraries.
- C compiler, in this case gcc was used.

Compiled with:
```bash
gcc -Wall -Wextra -o main main.c

```
## Running
To run just:
```bash
./main

```
The server listens on:  

http://localhost:8080  

You can test it with a browser or with curl:  

curl http://localhost:8080/  

## TODO
- Implement protection against path traversal.
- Improve request processing.
- Improve EINTR handling.
- Handle SIGPIPE.
- Improve error handling.
- Improve response headers (content-length, content/mime types, etc).
