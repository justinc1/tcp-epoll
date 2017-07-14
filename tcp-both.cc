// http://www.wangafu.net/~nickm/libevent-book/01_intro.html

/* For sockaddr_in */
#include <netinet/in.h>
/* For socket functions */
#include <sys/socket.h>
/* For fcntl */
#include <fcntl.h>

#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

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
#include <map>

#define DBG 1
#include "common.cc"

#define MAXEVENTS 64

class EventData {
public:
    epoll_event event;
    int recv_cnt;
    int send_cnt;

    //status ( *connect)(connection *);
    int (*read_cb)(EventData *evd);
    int (*write_cb)(EventData *evd);
public:
    EventData() {
        memset(&event, 0x00, sizeof(event));
        recv_cnt = 0;
        send_cnt = 0;
        read_cb = NULL;
        write_cb = NULL;
    }
};

std::map<int, EventData> evdata;
char* peername = NULL;
char* peerport = NULL;
char* myport = NULL;
int epoll_fd = -1;
struct epoll_event *events = NULL;

int socket_connected(EventData *evd);
int server_accept(EventData *evd);
int server_read(EventData *evd);
int server_write(EventData *evd);
int client_read(EventData *evd);
int client_write(EventData *evd);

int connect_socket() {
    debug("FYI\n");
    int cfd;

    cfd = -1;//create_and_connect (servername, port);
    if (cfd == -1)
        return 1;

    return 0;
}

int socket_connected(EventData *evd) {
    debug("FYI\n");
    evd->read_cb = NULL;//socket_read;

    //socket_write(evd);

    return 0;
/*
    connection *c = data;

    switch (sock.connect(c)) {
        case OK:    break;
        case ERROR: goto error;
        case RETRY: return;
    }

    http_parser_init(&c->parser, HTTP_RESPONSE);
    c->written = 0;

    aeCreateFileEvent(c->thread->loop, fd, AE_READABLE, socket_readable, c);
    aeCreateFileEvent(c->thread->loop, fd, AE_WRITABLE, socket_writeable, c);

    return;

  error:
    c->thread->errors.connect++;
    reconnect_socket(c->thread, c);
*/
}

int server_accept(EventData *evd) {
    debug("FYI\n");

    EventData evd1;
    int s;
    int afd = accept(evd->event.data.fd, NULL, NULL);
    assert(afd > 0);
    printf("SRV accept-ed afd=%d on sfd=%d\n\n", afd, evd->event.data.fd);

    evd1.read_cb = server_read;
    evd1.write_cb = server_write;
    evd1.event.data.fd = afd;
    evd1.event.events = EPOLLIN | EPOLLOUT | EPOLLET;
    evdata[afd] = evd1;
    s = epoll_ctl (epoll_fd, EPOLL_CTL_ADD, afd, &evdata[afd].event);
    if (s == -1)
    {
        perror ("epoll_ctl");
        abort ();
    }
    debug("epoll_fd=%d EPOLL_CTL_ADD afd=%d\n", epoll_fd, afd);

    return 0;
}

int server_read(EventData *evd) {
    debug("FYI\n");

    ssize_t count = 0, nr;
    char buf[512] = "-------------";
    ssize_t buflen = 0;
    while (1)
    {
        nr = read (evd->event.data.fd, buf+count, sizeof(buf)-count);
        if (nr == -1) {
            if (errno == EAGAIN) {
                debug ("nr=%zu EAGAIN", nr);
                continue;
            }
            else {
                debug("WTF ??? nr=%zu errno=%d\n", nr, errno);
            }
        }
        if (nr == 0) {
            debug ("EOF nr=%zu", nr);
            return 0;
        }
        if (nr < 0) {
            debug("WTF ??? nr=%zu\n", nr);
        }

        assert(nr > 0);
        count += nr;
        if( buf[count-1] == '\0' )
            break; // terminating 0x00 in string
    }
    buflen = strlen(buf) + 1;
    evd->recv_cnt++;
    printf("SRV recv (%d): %zu %s\n", evd->recv_cnt, buflen, buf);
    //sleep(1);
    return count;
}

int server_write(EventData *evd) {
    debug("FYI\n");

    // we can write data
    ssize_t count = 0;
    char buf[512] = "TEST-s-00";  // TODO http GET /
    ssize_t buflen = strlen(buf) + 1;
    while (1)
    {
        count += write (evd->event.data.fd, buf+count, buflen-count);
        printf(" part SRV sent (%d): %zu %s\n", evd->send_cnt, buflen, buf);
        if (count < buflen)
        {
            if (errno == EAGAIN)
            {
                perror ("write and EAGAIN");
            }
        }
        else if (count == buflen)
        {
            evd->send_cnt++;
            printf("SRV  sent (%d): %zu %s\n", evd->send_cnt, buflen, buf);
            //sleep(1);
            break;
        }
    }

    return count;
}

int client_read(EventData *evd) {
    debug("FYI\n");

    ssize_t count = 0;
    char buf[512] = "-------------";
    ssize_t buflen = 0;
    while (1)
    {
        count += read (evd->event.data.fd, buf+count, sizeof(buf)-count);
        if( buf[count-1] == '\0' )
            break; // terminating 0x00 in string
    }
    buflen = strlen(buf) + 1;
    evd->recv_cnt++;
    printf("CLNT recv (%d): %zu %s\n", evd->recv_cnt, buflen, buf);
    //sleep(1);
    return count;
}

int client_write(EventData *evd) {
    debug("FYI\n");

    // we can write data
    ssize_t count = 0;
    char buf[512] = "TEST-c-00";  // TODO http GET /
    ssize_t buflen = strlen(buf) + 1;
    while (1)
    {
        count += write (evd->event.data.fd, buf+count, buflen-count);
        printf(" part CLNT sent (%d): %zu %s\n", evd->send_cnt, buflen, buf);
        if (count < buflen)
        {
            if (errno == EAGAIN)
            {
                perror ("write and EAGAIN");
            }
        }
        else if (count == buflen)
        {
            evd->send_cnt++;
            printf("CLNT sent (%d): %zu %s\n", evd->send_cnt, buflen, buf);
            //sleep(1);
            break;
        }
    }

    return count;
}

int
main (int argc, char *argv[])
{
    int cfd, sfd, s;
    EventData evd1;

    if (argc != 4)
    {
        fprintf (stderr, "Usage: %s [peername peerport myport]\n", argv[0]);
        exit (EXIT_FAILURE);
    }

    peername = argv[1];
    peerport = argv[2];
    myport = argv[3];

    epoll_fd = epoll_create1 (0);
    if (epoll_fd == -1)
    {
        perror ("epoll_create");
        abort ();
    }
    debug("epoll_fd=%d\n", epoll_fd);

    /*------------------------------------------------------------------------*/
    // create server
    sfd = server_create(myport);
    if (sfd == -1)
        abort ();

    evd1.read_cb = server_accept; //socket_connected;
    evd1.write_cb = server_write;
    evd1.event.data.fd = sfd;
    evd1.event.events = EPOLLIN | /*EPOLLOUT |*/ EPOLLET;
    evd1.event.events = EPOLLIN | EPOLLOUT;// |*/ EPOLLET;
    evdata[sfd] = evd1;
    s = epoll_ctl (epoll_fd, EPOLL_CTL_ADD, sfd, &evdata[sfd].event);
    if (s == -1)
    {
        perror ("epoll_ctl");
        abort ();
    }
    debug("epoll_fd=%d EPOLL_CTL_ADD sfd=%d\n", epoll_fd, sfd);

    /*------------------------------------------------------------------------*/
    // create client
    cfd = client_create();
    if (cfd == -1)
        abort ();
    //usleep(10*1000);

    evd1.read_cb = client_read; //socket_connected;
    evd1.write_cb = client_write;
    evd1.event.data.fd = cfd;
    evd1.event.events = EPOLLIN | EPOLLOUT | EPOLLET;
    evdata[cfd] = evd1;
    s = epoll_ctl (epoll_fd, EPOLL_CTL_ADD, cfd, &evdata[cfd].event);
    if (s == -1)
    {
        perror ("epoll_ctl");
        abort ();
    }
    debug("epoll_fd=%d EPOLL_CTL_ADD cfd=%d\n", epoll_fd, cfd);

    /*------------------------------------------------------------------------*/
    /* Buffer where events are returned */
    events = (struct epoll_event*) calloc (MAXEVENTS, sizeof evd1.event);

    // wait on second VM (peer == server) to be up
    for (int ii=5; ii>0; --ii) {
        printf("Wait on peer %d\n", ii);
        sleep(1);
    }
    cfd = client_connect(cfd, peername, peerport);
    if (cfd == -1)
        abort ();

    /* The event loop */
    while (1)
    {
        int n, i;

        usleep(1000*1000*2);

        n = epoll_wait (epoll_fd, events, MAXEVENTS, -1);
        for (i = 0; i < n; i++)
        {
            debug("event[%d/%d] fd=%d events=%s\n", i, n, events[i].data.fd, epoll_event_to_str(events[i].events));
            EventData& evd = evdata[events[i].data.fd];

            if (events[i].events & EPOLLERR)
            {
                fprintf (stderr, "epoll error EPOLLERR\n");
                //close (events[i].data.fd);
            }
            if (events[i].events & EPOLLHUP)
            {
                fprintf (stderr, "epoll error EPOLLHUP\n");
                //close (events[i].data.fd);
            }

            if((events[i].events & EPOLLIN))
            {
                if (evd.read_cb) {
                    evd.read_cb(&evd);
                }
            }

            if((events[i].events & EPOLLOUT))
            {
                if (evd.write_cb) {
                    evd.write_cb(&evd);
                }
            }

        }// for (i=0:n)
    }//while 1

    free (events);

    close (cfd);

    return EXIT_SUCCESS;
}


