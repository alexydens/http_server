/* Elm headers */
#include <base.h>         /* Base includes */
#include <arena_alloc.h>  /* Linear allocator */
#include <str.h>          /* Length-based strings */

/* libc headers */
#define _GNU_SOURCE
#include <sys/socket.h>   /* For socket function definitions */
#include <netinet/in.h>   /* For sockaddr_in struct */
#include <stdio.h>        /* I/O related function definitions */
#include <unistd.h>       /* Misc function definitions */
#include <fcntl.h>        /* For non-blocking I/O */
#include <stdlib.h>       /* For exit() */
#include <string.h>       /* For strerror() */
#include <errno.h>        /* Error handling */

/* Macro consts */
#define MAX_CONNS         8
#define BUFF_SIZE         1024*1024*16
#define PORT              8080

/* State of application */
static struct {
  i32 sockfd;
  i32 clientfds[MAX_CONNS];
  i32 max_clients;
  char buff[BUFF_SIZE];
  struct sockaddr_in addr;
} server;

/* Normal consts */
static int option = 1;
static const char* request_start = 
"HTTP/1.1 200 OK\n"
"Content-Length: ";

/* Functions */
void parse_request(i32 conn);

/* Entry point */
int main(void) {
  /* Change input flags */
  printf("INFO: Setting input mode...\n");
  i32 flags = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

  /* Create socket */
  printf("INFO: Creating socket...\n");
  server.sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
  if (server.sockfd < 0) {
    fprintf(stderr, "FATAL: Error creating socket: %s\n", strerror(errno));
    exit(-1);
  }
  setsockopt(server.sockfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
  /* Bind socket: address: any, port: PORT */
  printf("INFO: Binding socket address and port...\n");
  memset(&server.addr, 0x00, sizeof(struct sockaddr_in));
  server.addr.sin_family = AF_INET;
  server.addr.sin_addr.s_addr = INADDR_ANY;
  server.addr.sin_port = htons(PORT);
  if (bind(
        server.sockfd,
        (struct sockaddr*)&server.addr,
        (socklen_t)sizeof(server.addr)
      ) < 0) {
    fprintf(stderr, "FATAL: Error binding socket: %s\n", strerror(errno));
    exit(-1);
  }
  /* Listen for connections */
  if (listen(server.sockfd, MAX_CONNS) < 0) {
    fprintf(
        stderr,
        "FATAL: Error listening for connection: %s\n",
        strerror(errno)
    );
    exit(-1);
  }
  printf("INFO: Listening on port: "STR(PORT)"...\n");
  

  /* Close socket */
  printf("INFO: Closing socket...\n");

  /* Reset flags */
  printf("INFO: Resetting input mode...\n");
  fcntl(STDIN_FILENO, F_SETFL, flags);
  return 0;
}
