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

struct FileInfo {
	char *path;
	long size;
	int ok;
}

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

static void
read_request_line(struct HTTPRequest *req, FILE *in){
  char buf[LINE_BUF_SIZE];
  char *push, *p;

  if(!fgets(buf, LINE_BUF_SIZE, in)) // fgets() read one line, until /n. get requestLine.
    log_exit("no request line");
  p = strchr(buf, ' ');
  if(!p)
    log_exit("parse error on request line (1): %s", buf);
  *p++ = '\0';
  req->method = xmalloc(p - buf);
  strcpy(req->method, buf); // strcpy() copy string, start from *buf to next '\0'(the mark for end of string).
  upcase(req->method);

  path = p;
  p = strchr(path, ' ');
  if (!p)
    log_exit("parse error on request line (2): %s", buf);
  *p++ = '\0';
  req->path = xmalloc(p - path);
  strcpy(req->path, path);

  if(strncasecmp(p, "HTTP/1.", strlen("HTTP/1.")) != 0)
    log_exit("parse error on request line (3): %s", buf);
  p += strlen("HTTP/1.");
  req->protocol_minor_version = atoi(p); //atoi() parse string to int.
}

static struct HTTPHeaderField*
read_header_field(FILE *in){
  struct HTTPHeaderField *h;
  char buf[LINE_BUF_SIZE];
  char *p;

  if(!fgets(buf, LINE_BUF_SIZE, in))
    log_exit("failed to read request header field: %s", strerror(errno));
  if(buf[0] == '\n' || (strcmp(buf, "\r\n") == 0))
    return NULL;

  // extract name.
  p = strchr(buf, ':');
  if (!p)
    log_exit("parse error on request header field: %s", buf);
  *p++ = '\0';
  h = xmalloc(sizeof(struct HTTPHeaderField));
  h->name = xmalloc(p - buf);
  strcpy(h->name, buf);

  // extract value.
  p += strspn(p, " \t"); //if header string containe extra ' ' or '\t'(tab), ahead pointer to its size for ignore them.
  h->value = xmalloc(strlen(p) + 1);
  strcpy(h->value, p);

  return h;
}

static long
content_length(struct HTTPRequest *req){
  char *val;
  long len;

  // if httpRequest has a body it also should have a header "Content-Length". 
  var = lookup_header_field_value(req, "Content-Length");
  if(!val)
    return 0;
  len = atol(val);
  if(len < 0)
    log_exit("negative Content-Length value");
  return len;
}

static char*
lookup_header_field_value(struct HTTPRequest *req, char *name){
  struct HTTPHeaderField *h;
  for(h = req->header; h; h = h->next){
    if(strcasecmp(h->name, name) == 0)
      return h->value;
  }
  return NULL;
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

static void
free_fileinfo (FileInfo *info) {
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
