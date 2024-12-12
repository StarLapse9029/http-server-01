#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <threads.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define PORT 8080
#define BUFFER_SIZE 1024


int start_server(socklen_t addr_len);
void handle_request(int client_fd);
void send_response(int client_fd);

int main(void){

  int server_fd, client_fd;
  struct sockaddr_in client_addr;
  socklen_t addr_len = sizeof(client_addr);
  server_fd = start_server(addr_len);
  
  while(1){
    sleep(5);
    client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
    if(client_fd == -1){
      perror("Accept failed");
      continue;
    }
    handle_request(client_fd);
  }
}

int start_server(socklen_t addr_len){
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if(server_fd == -1){
    goto sock_error;
  }
  
  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(PORT);
  server_addr.sin_family = AF_INET;

  if(bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1){
     goto sock_error;
  }
  if(listen(server_fd, 5) == -1){
     goto sock_error;
  }

  return server_fd;
  
  sock_error:
    if(server_fd != -1){
    close(server_fd);
  }
    perror("Socket error");
    exit(EXIT_FAILURE);
}

void handle_request(int client_fd){
  char buffer[BUFFER_SIZE] = {0};
  int bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
  if(bytes_read > 0){
    buffer[bytes_read] = '\0';
    printf("Received request:\n  %s\n", buffer);
    send_response(client_fd);
  }
}

void send_response(int client_fd){
    char *response = "HTTP/1.1 200 OK\r\n"
                     "Content-Type: text/html\r\n"
                     "Connection: close\r\n\r\n"
                     "<html><body><h1>Hello, world!</h1></body></html>\r\n";

    write(client_fd, response, strlen(response));
}

