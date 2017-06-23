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

public:
    EventData() {
        memset(&event, 0x00, sizeof(event));
        recv_cnt = 0;
        send_cnt = 0;
    }
};

std::map<int, EventData> evdata;

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

int
main (int argc, char *argv[])
{
    int sfd, s;
    int efd;
    EventData evd1;
    struct epoll_event *events;

    if (argc != 3)
    {
        fprintf (stderr, "Usage: %s [servername port]\n", argv[0]);
        exit (EXIT_FAILURE);
    }

    sfd = create_and_connect (argv[1], argv[2]);
    if (sfd == -1)
        abort ();

    s = make_socket_non_blocking (sfd);
    if (s == -1)
        abort ();

    efd = epoll_create1 (0);
    if (efd == -1)
    {
        perror ("epoll_create");
        abort ();
    }

    evd1.event.data.fd = sfd;
    evd1.event.events = EPOLLIN | EPOLLOUT | EPOLLET;
    s = epoll_ctl (efd, EPOLL_CTL_ADD, sfd, &evd1.event);
    if (s == -1)
    {
        perror ("epoll_ctl");
        abort ();
    }

    evdata[sfd] = evd1;

    /* Buffer where events are returned */
    events = (struct epoll_event*) calloc (MAXEVENTS, sizeof evd1.event);

    /* The event loop */
    while (1)
    {
        int n, i;

        n = epoll_wait (efd, events, MAXEVENTS, -1);
        for (i = 0; i < n; i++)
        {
            debug("event[%d/%d] fd=%d events=%s\n", i, n, events[i].data.fd, epoll_event_to_str(events[i].events));
            EventData& evd = evdata[events[i].data.fd];

            if (events[i].events & EPOLLERR)
            {
                fprintf (stderr, "epoll error EPOLLERR\n");
                close (events[i].data.fd);
            }
            if (events[i].events & EPOLLHUP)
            {
                fprintf (stderr, "epoll error EPOLLHUP\n");
                close (events[i].data.fd);
            }

            if((events[i].events & EPOLLIN))
            {
                ssize_t count = 0;
                char buf[512] = "-------------";
                ssize_t buflen = 0;
                while (1)
                {
                    count += read (events[i].data.fd, buf+count, sizeof(buf)-count);
                    if( buf[count-1] == '\0' )
                        break;
                }
                buflen = strlen(buf) + 1;
                evd.recv_cnt++;
                printf("CLNT recv (%d): %zu %s\n", evd.recv_cnt, buflen, buf);
                //sleep(1);
            }

            if((events[i].events & EPOLLOUT))
            {
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
            }
        }// for (i=0:n)
    }//while 1

    free (events);

    close (sfd);

    return EXIT_SUCCESS;
}


