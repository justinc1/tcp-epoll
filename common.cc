
#ifndef DBG
# define DBG 1
#endif

# define debug(fmt, ...) if(DBG) { fprintf(stderr, "%s:%d %s: " fmt, __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__); }
//# define debug(...) /**/

//https://banu.com/blog/2/how-to-use-epoll-a-complete-example-in-c/epoll-example.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <errno.h>
#include <netinet/tcp.h>

#define MAXEVENTS 64

int
make_socket_non_blocking (int sfd)
{
  int flags, s;

  flags = fcntl (sfd, F_GETFL, 0);
  if (flags == -1)
    {
      perror ("fcntl");
      return -1;
    }

  flags |= O_NONBLOCK;
  s = fcntl (sfd, F_SETFL, flags);
  if (s == -1)
    {
      perror ("fcntl");
      return -1;
    }

  return 0;
}

int
create_and_bind (char *port)
{
  struct addrinfo hints;
  struct addrinfo *result, *rp;
  int s, sfd;

  memset (&hints, 0, sizeof (struct addrinfo));
  hints.ai_family = AF_INET;     /* Return IPv4 and IPv6 choices */
  hints.ai_socktype = SOCK_STREAM; /* We want a TCP socket */
  hints.ai_flags = AI_PASSIVE;     /* All interfaces */

  s = getaddrinfo (NULL, port, &hints, &result);
  if (s != 0)
    {
      fprintf (stderr, "getaddrinfo: %s\n", gai_strerror (s));
      return -1;
    }

  for (rp = result; rp != NULL; rp = rp->ai_next)
    {
      sfd = socket (rp->ai_family, rp->ai_socktype, rp->ai_protocol);
      if (sfd == -1)
        continue;

      s = bind (sfd, rp->ai_addr, rp->ai_addrlen);
      if (s == 0)
        {
          /* We managed to bind successfully! */
          break;
        }

      close (sfd);
    }

  if (rp == NULL)
    {
      fprintf (stderr, "Could not bind\n");
      return -1;
    }

  freeaddrinfo (result);

  return sfd;
}

int
server_create(char *port)
{
    int s, sfd = -1;

    sfd = create_and_bind (port);
    if (sfd == -1) {
        goto ERR;
    }
    debug("sfd=%d\n", sfd);

    s = make_socket_non_blocking (sfd);
    if (s == -1) {
        goto ERR;
    }

    s = listen (sfd, SOMAXCONN);
    if (s == -1)
    {
        perror ("listen");
        goto ERR;
    }

    return sfd;

ERR:
    if (sfd != -1) {
        close(sfd);
    }
    sfd = -1;
    return sfd;
}

int
client_create()
//create_and_connect(char *servername, char *port)
{
  int s, cfd;
  int flags;

  cfd = socket (AF_INET, SOCK_STREAM, 0);
  if (cfd == -1) {
      fprintf (stderr, "socket() failed\n");
      goto ERR;
  }
  debug("cfd=%d\n", cfd);

  flags = fcntl(cfd, F_GETFL, 0);
  s = fcntl(cfd, F_SETFL, flags | O_NONBLOCK);
  debug("fcntl F_SETFL s=%d\n", s);
  if (s != 0) {
    fprintf (stderr, "fcntl() failed\n");
    goto ERR;
  }

  flags = 1;
  s = setsockopt(cfd, IPPROTO_TCP, TCP_NODELAY, &flags, sizeof(flags));
  if (s != 0) {
    fprintf (stderr, "setsockopt() failed\n");
    goto ERR;
  }

  return cfd;

ERR:
  close (cfd);
  return -1;
}

int
client_connect (int cfd, char *servername, char *port)
{
  struct addrinfo hints;
  struct addrinfo *result, *rp;
  int s;

  memset (&hints, 0, sizeof (struct addrinfo));
  hints.ai_family = AF_INET;     /* Return IPv4 and IPv6 choices */
  hints.ai_socktype = SOCK_STREAM; /* We want a TCP socket */
  hints.ai_flags = AI_PASSIVE;     /* All interfaces */

  s = getaddrinfo (servername, port, &hints, &result);
  if (s != 0)
    {
      fprintf (stderr, "getaddrinfo: %s\n", gai_strerror (s));
      return -1;
    }

  for (rp = result; rp != NULL; rp = rp->ai_next)
    {

      s = connect (cfd, rp->ai_addr, rp->ai_addrlen);
      if (s != 0 && errno != EINPROGRESS)
      {
          goto ERR;
      }
      debug("cfd=%d is connected\n", cfd);

      /* We managed to connect successfully! */
      goto OK;
    }

ERR:
  close (cfd);
  cfd = -1;

OK:
  if (rp == NULL)
    {
      fprintf (stderr, "Could not connect\n");
      return -1;
    }

  freeaddrinfo (result);

  return cfd;
}

char* epoll_event_to_str(int event) {
    static char buf[512];
    memset(buf, 0x00, sizeof(buf));
    int pos=0;
    pos += snprintf(buf+pos, sizeof(buf)-pos, "0x%02x EPOLL{", event);
    if (event & EPOLLIN) {
        pos += snprintf(buf+pos, sizeof(buf)-pos, " IN");
    }
    if (event & EPOLLOUT) {
        pos += snprintf(buf+pos, sizeof(buf)-pos, " OUT");
    }
    if (event & EPOLLERR) {
        pos += snprintf(buf+pos, sizeof(buf)-pos, " ERR");
    }
    if (event & EPOLLHUP) {
        pos += snprintf(buf+pos, sizeof(buf)-pos, " HUP");
    }
    pos += snprintf(buf+pos, sizeof(buf)-pos, " }");
    return buf;
}
