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
char* servername = NULL;
char* port = NULL;

int socket_connected(EventData *evd);
int socket_read(EventData *evd);
int socket_write(EventData *evd);

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
    evd->read_cb = socket_read;

    socket_write(evd);

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

int socket_read(EventData *evd) {
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

int socket_write(EventData *evd) {
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
    int cfd, s;
    int efd;
    EventData evd1;
    struct epoll_event *events;

    if (argc != 3)
    {
        fprintf (stderr, "Usage: %s [servername port]\n", argv[0]);
        exit (EXIT_FAILURE);
    }

    servername = argv[1];
    port = argv[2];
    cfd = client_create();
    if (cfd == -1)
        abort ();
    //usleep(10*1000);

    efd = epoll_create1 (0);
    if (efd == -1)
    {
        perror ("epoll_create");
        abort ();
    }

    evd1.event.data.fd = cfd;
    evd1.event.events = EPOLLIN | EPOLLOUT | EPOLLET;
    s = epoll_ctl (efd, EPOLL_CTL_ADD, cfd, &evd1.event);
    if (s == -1)
    {
        perror ("epoll_ctl");
        abort ();
    }

    evd1.read_cb = socket_read; //socket_connected;
    evd1.write_cb = socket_write;

    evdata[cfd] = evd1;

    /* Buffer where events are returned */
    events = (struct epoll_event*) calloc (MAXEVENTS, sizeof evd1.event);

    cfd = client_connect(cfd, servername, port);
    if (cfd == -1)
        abort ();


    /* The event loop */
    while (1)
    {
        int n, i;

        usleep(1000*500);

        n = epoll_wait (efd, events, MAXEVENTS, -1);
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
                /*
                ssize_t count = 0;
                char buf[512] = "-------------";
                ssize_t buflen = 0;
                while (1)
                {
                    count += read (events[i].data.fd, buf+count, sizeof(buf)-count);
                    if( buf[count-1] == '\0' )
                        break; // aha, connect ...
                }
                buflen = strlen(buf) + 1;
                evd.recv_cnt++;
                printf("CLNT recv (%d): %zu %s\n", evd.recv_cnt, buflen, buf);
                //sleep(1);
                */
            }

            if((events[i].events & EPOLLOUT))
            {
                if (evd.write_cb) {
                    evd.write_cb(&evd);
                }
/*
                // we can write data
                ssize_t count = 0;
                char buf[512] = "TEST-c-00";  // TODO http GET /
                ssize_t buflen = strlen(buf) + 1;
                while (1)
                {
                    count += write (events[i].data.fd, buf+count, buflen-count);
                    if (count < buflen)
                    {
                        if (errno == EAGAIN)
                        {
                            perror ("write and EAGAIN");
                        }
                    }
                    else if (count == buflen)
                    {
                        evd.send_cnt++;
                        printf("CLNT sent (%d): %zu %s\n", evd.send_cnt, buflen, buf);
                        //sleep(1);
                        break;
                    }
                }
*/
            }
        }// for (i=0:n)
    }//while 1

    free (events);

    close (cfd);

    return EXIT_SUCCESS;
}


