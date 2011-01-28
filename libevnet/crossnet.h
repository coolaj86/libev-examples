#include <sys/socket.h>
#ifdef MSG_NOSIGNAL
#define EVN_NOSIGNAL MSG_NOSIGNAL
#else
#define EVN_NOSIGNAL SO_NOSIGPIPE
#endif
