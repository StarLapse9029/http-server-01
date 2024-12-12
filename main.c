#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main(){
  int server_fd, client_fd;
  struct sockaddr_in server_addr, client_addr;
  char buffer[BUFFER_SIZE];
    
  socklen_t addr_len = sizeof(client_addr);
  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if(server_fd == -1){
    perror("Socket creation failed");
    exit(EXIT_FAILURE);
  }

  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(PORT);

  if(bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1){
    perror("Bind failed");
    close(server_fd);
    exit(EXIT_FAILURE);
  }

  if(listen(server_fd, 5) == -1){
    perror("Listen failed");
    close(server_fd);
    exit(EXIT_FAILURE);
  }

  printf("Serve listening on port %d\n", PORT);

  while(1){
    client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
    if(client_fd == -1){
      perror("Accept failed");
      continue;
    }

    printf("Connection established with client\n");
  
  int bytes_read = read(client_fd, buffer, sizeof(buffer) -1);
  if(bytes_read > 0){
    buffer[bytes_read] = '\0';
    printf("received:\n    %s\n", buffer);
    char *response = "HTTP/1.1 200 OK\r\n"
                     "Content-Type: text/html\r\n"
                     "Connection: close\r\n\r\n"
                     "<html><body><h1>Hello, world!</h1></body></html>\r\n";

    write(client_fd, response, strlen(response));
    }
  }

  close(client_fd);
  close(server_fd);

  return 0;
}
