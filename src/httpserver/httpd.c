/****** Constants ********************************************************/

#define SERVER_NAME "LittleHTTP"
#define SERVER_VERSION "1.0"
#define HTTP_MINOR_VERSION 0
#define BLOCK_BUF_SIZE 1024
#define LINE_BUF_SIZE 4096
#define MAX_REQUEST_BODY_LENGTH (1024 * 1024)



/****** Data Type Definitions ********************************************/

/* it is copy of the HTTPRequest's structure. see P358,368. */
struct HTTPRequest {
    int protocol_minor_version;
    char *method;
    char *path;
    struct HTTPHeaderField *header;
    char *body;
    long length;
};

struct HTTPHeaderField {
    char *name;
    char *value;
    struct HTTPHeaderField *next;
};


/****** Functions ******************************************************/ 

/* argv[1] needs docroot path. */
int
main(int argc, char *argv[]){
  if(argc != 2){
    fprintf(stderr, "Usage: %s <docroot>\n", argv[0]); /* [0] is program name. */
    exit(1);
  }
  install_signal_handlers(); /* set custom sighandler. */
  service(stdin, stdout, argv[1]);
  exit(0);
}

/* exec frame */
static void
service(FILE *in, FILE *out, char *docroot){
  struct HTTPRequest *req;

  req = read_request(in);
  respond_to(req, out, docroot);
  free_request(req);
}

/* return struct's point. */
static struct HTTPRequest*
read_request(FILE *in){
  struct HTTPRequest *req;
  struct HTTPHeaderField *h;

  req = xmalloc(sizeof(struct HTTPRequest));
  // extract requestLine's data.
  read_request_line(req, in);
  // extract httpHeaders.
  req->header = NULL;
  while(h = read_header_field(in)){
    h->next = req->header;
    req->header = h;
  }
  // get content length.
  req->length = content_length(req);
  // extract content body.
  if(req->length != 0){
    if(req->length > MAX_REQUEST_BODY_LENGTH)
      log_exit("request body too long");
    req->body = xmalloc(req->length);
    if(fread(req->body, req->length, 1, in) < 1) //cf.fread) http://bituse.info/c_func/43
      log_exit("failed to read request body");
  }els{
    req->body = NULL;
  }
  return req;
}

/* *cahr and *struct should be freeed. */
static void
free_request(struct HTTPRequest *req){
  struct HTTPHeaderField *h, *head;

  head = req->header;
  while(head){
    h = head;
    head = head->next;
    free(h->name);
    free(h->value);
    free(h);
  }
  free(req->method);
  free(req->path);
  free(req->body);
  free(req);
}

/****** utils *********************************************************/

/**** trap signal ****/
static void
install_signal_handlers(void) {
  trap_signal(SIGPIPE, signal_exit);
}

static void
trap_signal(int sig, sighandler_t handler) {
  struct sigaction act;
  act.sa_handler = handler;
  sigemptyset(&act.sa_mask);
  act.sa_flags = SA_RESTART;
  if(sigaction(sig, &act, NULL) < 0)
    log_exit("sigaction() failed: %s", strerror(errno));
}

static void
signal_exit(int sig) {
  log_exit("exit by sisgnal %d", sig);
}

/**** alloc memory ****/

static void*
xmalloc(size_t sz) {
  void *p;
  if(!P = malloc(sz)) log_exit("failed to alloc memory.");
  return p;
}

/**** error exit ****/

static void
log_exit(const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    if (debug_mode) {
        vfprintf(stderr, fmt, ap);
        fputc('\n', stderr);
    }
    else {
#if defined(HAVE_VSYSLOG)
        vsyslog(LOG_ERR, fmt, ap);
#elif defined(HAVE_VSNPRINTF)
        char buf[1024];

        vsnprintf(buf, 1024, fmt, ap);
        syslog(LOG_ERR, buf);
#else
# error "vsyslog or vsnprintf is required"
#endif
    }
    va_end(ap);
    exit(1);
}
