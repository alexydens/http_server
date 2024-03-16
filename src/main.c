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
#define BUFF_SIZE         1024*1024
#define PORT              8080

/* State of application */
static struct {
  i32 sockfd;
  i32 clientfds[MAX_CONNS];
  i32 num_clients;
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
  /* Listen for clientfds */
  if (listen(server.sockfd, MAX_CONNS) < 0) {
    fprintf(
        stderr,
        "FATAL: Error listening for connection: %s\n",
        strerror(errno)
    );
    exit(-1);
  }
  printf("INFO: Listening on port: %d...\n", PORT);
  
  /* Accept first connection */
  server.num_clients = 0;
  while (1) {
    int i;
    char c;
    if (read(STDIN_FILENO, &c, 1) && c == 'q') break;
    server.clientfds[server.num_clients] =
      accept4(server.sockfd, NULL, NULL, SOCK_NONBLOCK);
    if (server.clientfds[server.num_clients] > 0) {
      memset(server.buff, 0x00, BUFF_SIZE);
      recv(
          server.clientfds[server.num_clients],
          server.buff,
          BUFF_SIZE-1,
          0
      );
      parse_request(server.clientfds[server.num_clients]);
      server.num_clients++;
    }
    for (i = 0; i < server.num_clients; i++) {
      memset(server.buff, 0x00, BUFF_SIZE);
      if (recv(
          server.clientfds[i],
          server.buff,
          BUFF_SIZE-1,
          0
      ) > 1) {
        parse_request(server.clientfds[i]);
      }
    }
  }

  /* Close socket */
  printf("INFO: Closing socket...\n");

  /* Reset flags */
  printf("INFO: Resetting input mode...\n");
  fcntl(STDIN_FILENO, F_SETFL, flags);
  return 0;
}

/* Functions */
void parse_request(i32 conn) {
  /* For iteration */
  u64 i;
  /* For allocation */
  arena_t arena;
  /* The request - parsed */
  str_t request;
  str_t rel_path;
  arena = create_arena(NULL, 1024*1024*16);
  /* Parse the request */
  request.size = 0;
  for (i = 0; i < 9; i++) {
    if (server.buff[i] == ' ') break;
    request.size++;
  }
  request.ptr = arena_alloc(&arena, request.size);
  memcpy(request.ptr, server.buff, request.size);
  rel_path.size = 0;
  for (i = request.size+1; i < 48 + request.size; i++) {
    if (server.buff[i] == ' ') break;
    rel_path.size++;
  }
  rel_path.ptr = arena_alloc(&arena, rel_path.size);
  memcpy(rel_path.ptr, server.buff+request.size+1, rel_path.size);

  /* Log request */
  printf("REQUEST:\n\tTYPE: ");
  print_string(request);
  printf("\n\tPATH: ");
  print_string(rel_path);
  printf("\n");

  /* Send a response */
  if (string_compare(request, string_from(&arena, "GET"))) {
    str_t full_path;
    if (string_compare(rel_path, string_from(&arena, "/"))) {
      full_path = string_from(&arena, "server/index.html\0");
    } else {
      str_t null = string_from(&arena, "0");
      str_t tmp = string_concat(
          &arena,
          string_from(&arena, "server"),
          rel_path
      );
      null.ptr[0] = '\0';
      full_path = string_concat(&arena, tmp, null);
    }
    printf("full_path = %s\n", full_path.ptr);
    u64 bytes_msglen;
    char msglen[48];
    str_t file = string_file(&arena, full_path.ptr);
    bytes_msglen = sprintf(msglen, "%d\n\n", file.size);
    memset(server.buff, 0x00, BUFF_SIZE);
    memcpy(server.buff, request_start, 32);
    memcpy(server.buff+32, msglen, bytes_msglen);
    memcpy(server.buff+32+bytes_msglen, file.ptr, file.size);
    server.buff[32+bytes_msglen+file.size] = '\0';
    send(conn, server.buff, strlen(server.buff), 0);
  }

  /* Free arena */
  arena_free(&arena);
}
