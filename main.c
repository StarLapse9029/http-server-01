#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/sendfile.h>

#define PORT 8080
#define ADDRESS INADDR_ANY
#define BUFFER_SIZE 1024



// Used by SIGINT handler: set to zero to stop main loop
static volatile sig_atomic_t running = 1;

void erro(const char *e, int err_no);
void child(int client_fd);
void handleRequest(char* req, int client_fd);
void handleGet(char* path, int client_fd);
void handleHead(char* path, int client_fd);
void handlePost(char* path, int client_fd);
void handlePut(char* path, int client_fd);
void handleDelete(char* path, int client_fd);
void handleConnect(char* path, int client_fd);
void handleOptions(char* path, int client_fd);
void handleTrace(char* path, int client_fd);
void handlePatch(char* path, int client_fd);

typedef enum{
  GET,
  HEAD,
  POST,
  PUT,
  DELETE,
  CONNECT,
  OPTIONS,
  TRACE,
  PATCH,
  COUNT
} HttpMethod;

typedef void (*HttpHandler)(char *, int);
static const HttpHandler handlers[COUNT] = {
  [GET] = handleGet,
  [HEAD] = handleHead,
  [POST] = handlePost,
  [PUT] = handlePut,
  [DELETE] = handleDelete,
  [CONNECT] = handleConnect,
  [OPTIONS] = handleOptions,
  [TRACE] = handleTrace,
  [PATCH] = handlePatch
};

HttpMethod parseMethod(const char *method){
    static const char *names[COUNT] = {
        [GET]     = "GET",
        [HEAD]    = "HEAD",
        [POST]    = "POST",
        [PUT]     = "PUT",
        [DELETE]  = "DELETE",
        [CONNECT] = "CONNECT",
        [OPTIONS] = "OPTIONS",
        [TRACE]   = "TRACE",
        [PATCH]   = "PATCH"
    };

    for (int i = 0; i < COUNT; i++) {
        if (strcmp(method, names[i]) == 0) {
            return i;
        }
    }
    return COUNT; // invalid method
}

void handleSigint(int sig){
    (void)sig; // This parameter is unused
    running = 0; // Stop main loop
}

int main(void){
    // Signal handler for SIGINT
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa)); 
    sa.sa_handler = handleSigint; // Sets handleSigint as the handlers
    sigemptyset(&sa.sa_mask); // Prevent from blocking signals 
                              // while handler is running

    // Register the configuration for SIGINT
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        erro("sigaction", errno);
        return EXIT_FAILURE;
    }

    // Signal handler for SIGCHLD
    struct sigaction sa_chld;
    memset(&sa_chld, 0, sizeof(sa_chld));
    sa_chld.sa_handler = SIG_IGN; // Ignores the signal
    sa_chld.sa_flags = SA_NOCLDWAIT; // Handle child termination
                                     
    // Register the configuration for SIGCHLD
    if (sigaction(SIGCHLD, &sa_chld, NULL) == -1) {
        erro("sigaction SIGCHLD", errno);
        return EXIT_FAILURE;
    }
    puts("Starting Server");

    // Socket
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        erro("Socket", errno);
        return EXIT_FAILURE;
    }
    //puts("Socket: OK.");
    
    // Sets SO_REUSEADDR: can reuse a local address on certain conditions
    int opt = 1;
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        erro("setsockopt", errno);
        close(sock_fd);
        return EXIT_FAILURE;
    }

    struct sockaddr_in addr = {
        .sin_family = AF_INET, // Set for IPV4
        .sin_port = htons(PORT), // Set port
        .sin_addr.s_addr = htonl(ADDRESS) // Set address 
    };

    // Bind
    if (bind(sock_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        erro("Bind", errno);
        close(sock_fd);
        return EXIT_FAILURE;
    }
    //puts("Bind: OK.");

    // Listen
    if (listen(sock_fd, 10) == -1) {
        erro("Listen", errno);
        close(sock_fd);
        return EXIT_FAILURE;
    }
    //puts("Listen: OK.");
    printf("Listening on: %i\n", PORT);

    while (running) {
      
      int client_fd = accept(sock_fd, NULL, NULL);

      if (client_fd == -1) {
          if (errno == EINTR && !running) {
              break;
          }

          if (errno == EINTR) {
              continue;
          }

          erro("Accept", errno);
          break;
      }

      printf("Accepted - %d: OK.\n", client_fd);
      
      // Fork
      pid_t pid = fork();

      if (pid == -1) {
          erro("Fork", errno);
          close(client_fd);
          continue;
      }
      
      // If child proccess
      if (pid == 0) {
          close(sock_fd);
          child(client_fd);
          _exit(EXIT_SUCCESS);
      }
      // Continue parent proccess
      close(client_fd);
    }

    // Cleanup
    close(sock_fd);
    puts("\nCleanup: OK.");

    return EXIT_SUCCESS;
}

void erro(const char *e, int err_no){
    fprintf(stderr, "%s errno - %d\n%s\n",
            e, err_no, strerror(err_no));
}

// On child process
void child(int client_fd){

    char buffer[BUFFER_SIZE];
    // Receve messages from client_fd into buffer
    ssize_t req_size = recv(
        client_fd,
        buffer,
        sizeof(buffer) - 1,
        0
    );
    if (req_size > 0) {
        // If received 1 or more bytes
        buffer[req_size] = '\0';
        handleRequest(buffer, client_fd);
    }
    else if (req_size == 0) {
        puts("Client closed the connection.");
    }
    else {
        erro("Recv", errno);
    }
    close(client_fd);
}

void handleRequest(char* req, int client_fd){
    
    // Try to parse request
    char *method = strtok(req, " ");
    char *path   = strtok(NULL, " ");
    char *version = strtok(NULL, " ");

    if (method == NULL || path == NULL || version == NULL) {
        printf("Invalid HTTP request\n");
        return;
    }

    printf("Method:  %s\n", method);
    printf("Path:    %s\n", path);
    printf("Version: %s\n", version);
  
    HttpMethod parsed = parseMethod(method);
    if(parsed >= COUNT){
      const char *response =
          "HTTP/1.1 405 Method Not Allowed\r\n"
          "Content-Type: text/html\r\n"
          "Allow: GET, HEAD, POST, PUT, DELETE, CONNECT, OPTIONS, TRACE, PATCH\r\n"
          "Connection: close\r\n"
          "\r\n"
          "<h1>405 Method Not Allowed</h1>\r\n";

      write(client_fd, response, strlen(response));
      return;
    }
    handlers[parsed](path, client_fd);
}

void handleGet(char* path, int client_fd){
  char *header = "HTTP/1.1 200 OK\r\n"
                 "Content-Type: text/html\r\n"
                 "Connection: close\r\n\r\n";
  
  // Set path of requested file
  char file[BUFFER_SIZE];
  if(strcmp(path, "/") == 0){
    path = "index.html";
  }
  else{
    path++;
  }
  snprintf(file, sizeof(file), "./%s", path);

  // Try to open requested file
  int file_fd = open(file, O_RDONLY);
  // Handle file not found
  if (file_fd == -1) {
      const char *response =
          "HTTP/1.1 404 Not Found\r\n"
          "Content-Type: text/html\r\n"
          "Connection: close\r\n"
          "\r\n"
          "<h1>404 Not Found</h1>\r\n";

      write(client_fd, response, strlen(response));
      return;
  }

  // Write header
  write(client_fd, header, strlen(header));
  //char* data = "<html><body><h1>Hello, world!</h1></body></html>\r\n";
  
  char buffer[BUFFER_SIZE];
  ssize_t i;
 
  // Reads file in chunks
  while((i = read(file_fd, buffer, sizeof(buffer))) > 0){
    ssize_t j = 0;
    // Since write can write less bytes than requested
    // Need to keep track how many bytes were written
    while (j < i){
      ssize_t written = write(client_fd, buffer + j, i - j);
      if(written == -1){
        close(file_fd);
        return;
      }
      j += written;
    }
  }
  close(file_fd);            
}

void handleHead(char* path, int client_fd){
  (void)path;
  (void)client_fd;
  puts("HEAD");
}
void handlePost(char* path, int client_fd){
  (void)path;
  (void)client_fd;
  puts("POST");
}
void handlePut(char* path, int client_fd){
  (void)path;
  (void)client_fd;
    puts("PUT");
}
void handleDelete(char* path, int client_fd){
  (void)path;
  (void)client_fd;
    puts("DELETE");
}
void handleConnect(char* path, int client_fd){
  (void)path;
  (void)client_fd;
    puts("CONNECT");
}
void handleOptions(char* path, int client_fd){
  (void)path;
  (void)client_fd;
    puts("OPTIONS");
}
void handleTrace(char* path, int client_fd){
  (void)path;
  (void)client_fd;
    puts("TRACE");
}
void handlePatch(char* path, int client_fd){
  (void)path;
  (void)client_fd;
    puts("PATCH");
}


