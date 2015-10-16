#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

static int open_connection(char *host, char *service);

int
main(int argc, char *argv[]) {
  int sock;
  FILE *f;
  char buf[1024];

  sock = open_connection((argc > 1 ? argv[1]:"localhost"), "daytime");
  /* P125. Wrap file descriptor with FILE. */
  if(!f = fdopen(sock, "r")) {
    perror("fdopen(3)");
    exit(1);
  }
  fgets(buf, sizeof buf, f);
  fclose(f);
  fputs(buf, stdout);
  exit(0);
}

static int
open_connection(char *host, char *service){
  int sock;
  struct addrinfo hints, *res, *ai; /* res will have a Pointer of linkedList's head. */
  int err;

  /* init hints  */
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  /* P338 */
  if((err = getaddrinfo(host, service, &hints, &res)) != 0){ /* &res is DOUBLE Pointer */
    fprintf(stderr,  "getaddrinfo(3): %s\n", gai_strerror(err));
    exit(1);
  }
  /* "->" is arrow operator, access struct's menber via Pointer.
     see http://www.isl.ne.jp/pcsp/beginC/C_Language_14.html */
  for(ai = res; ai; ai = ai->ai_next){
    sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (sock < 0) {
      continue;
    }
    if(connect(sock, ai->ai_addr, ai->ai_addrlen) < 0){
      close(sock);
      continue;
    }
    /* success */
    freeaddrinfo(res);
    return sock;
  }

  /* all candidates are failed. */
  fprintf(stderr, "socket(2)/connect(2) failed");
  freeaddrinfo(res);
  exit(1);
}
