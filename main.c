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
#define BUFFER_SIZE 1024

static volatile sig_atomic_t running = 1;

void erro(const char *e, int err_no);
void child(int client_fd);
void handleRequest(char* req, int client_fd);
void handleGet(char* path, int client_fd);

void handleSigint(int sig)
{
    (void)sig;
    running = 0;
}

int main(void){
    // Signal handler
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handleSigint;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        erro("sigaction", errno);
        return EXIT_FAILURE;
    }

    struct sigaction sa_chld;
    memset(&sa_chld, 0, sizeof(sa_chld));
    sa_chld.sa_handler = SIG_IGN;
    sa_chld.sa_flags = SA_NOCLDWAIT;

    if (sigaction(SIGCHLD, &sa_chld, NULL) == -1) {
        erro("sigaction SIGCHLD", errno);
        return EXIT_FAILURE;
    }

    // Socket
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        erro("Socket", errno);
        return EXIT_FAILURE;
    }
    puts("Socket: OK.");

    int opt = 1;
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        erro("setsockopt", errno);
        close(sock_fd);
        return EXIT_FAILURE;
    }

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT),
        .sin_addr.s_addr = htonl(INADDR_ANY)
    };

    printf("Address - PORT %i: OK.\n", PORT);

    // Bind
    if (bind(sock_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        erro("Bind", errno);
        close(sock_fd);
        return EXIT_FAILURE;
    }
    puts("Bind: OK.");

    // Listen
    if (listen(sock_fd, 10) == -1) {
        erro("Listen", errno);
        close(sock_fd);
        return EXIT_FAILURE;
    }
    puts("Listen: OK.");

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

        printf("Accept - %d: OK.\n", client_fd);

        pid_t pid = fork();

        if (pid == -1) {
            erro("Fork", errno);
            close(client_fd);
            continue;
        }

        if (pid == 0) {
            close(sock_fd);
            child(client_fd);
            _exit(EXIT_SUCCESS);
        }
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

void child(int client_fd){
    char buffer[BUFFER_SIZE];
    ssize_t req_size = recv(
        client_fd,
        buffer,
        sizeof(buffer) - 1,
        0
    );
    if (req_size > 0) {
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

    if(strcmp(method, "GET") == 0){
      handleGet(path, client_fd);
    }
}

void handleGet(char* path, int client_fd){
  char *header = "HTTP/1.1 200 OK\r\n"
                 "Content-Type: text/html\r\n"
                 "Connection: close\r\n\r\n";
  char file[BUFFER_SIZE];
  if(strcmp(path, "/") == 0){
    path = "index.html";
  }
  else{
    path++;
  }
  snprintf(file, sizeof(file), "./%s", path);

  int file_fd = open(file, O_RDONLY);
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
  //Write data
  while((i = read(file_fd, buffer, sizeof(buffer))) > 0){
    ssize_t j = 0;
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
