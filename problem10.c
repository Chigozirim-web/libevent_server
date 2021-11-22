
#define _POSIX_C_SOURCE 200809L

#include "player.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <err.h>
#include <fcntl.h>

//libevent
#include <event2/event.h>
#include <event2/event_struct.h>


#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#define PORT 8000


static struct event readEvent, writeEvent, serverEvent;
static struct event_base *base;
static player_t *p;
static int listen_fd, ip6_fd;
static char *msg;

/* HELPER FUNCTIONS */
int setnonblock(int fd);
void ServerRead (int fd, short ev, void * arg);
void ServerWrite (int fd, short ev, void * arg);
void ServerAccept (int fd, short ev, void * arg);


int main(int argc, char* argv[])
{
    int port, on, on6;
    struct sockaddr_in listen_addr;
    struct sockaddr_in6 ip6_addr;
    
    base = event_base_new();

    if (argc == 1)
    {
        port = PORT;
    }
    else if(strcmp(argv[1], "-p") == 0)
    {
        port = atoi(argv[2]);  
    }
    else
    {
        printf("Expected format: ./gwgd [-p port] \n");
        return -1;
    }

    if(port < 0)
    {
        fprintf(stderr, "Invalid port number input\n");
        return -1;
    } 

    
    p = player_new();
    if (! p) 
    {
        perror("player_t");
        return -1;
    }

    /****************************
    *   FOR IPV4 ADDRESSES
    ****************************/
	listen_fd = socket (AF_INET, SOCK_STREAM, 0);
	if(listen_fd < 0)
    {
        perror("socket()");
        return -1;
    }

	//Empty memory data
	memset (&listen_addr, 0, sizeof (listen_addr));

	listen_addr.sin_family = AF_INET;
	listen_addr.sin_port = htons(port);
    listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	
	//bind function is used to bind the socket to a known address
	if (bind (listen_fd, (struct sockaddr *) &listen_addr, sizeof (listen_addr)) < 0) 
    {
		close (listen_fd);
		fprintf (stderr, "bind (): can not bind socket");
		return -1;
	}
	
	if (listen (listen_fd, 5) < 0) 
    {
		fprintf (stderr, "listen (): can not listen socket");
		close (listen_fd);
		return -1;
	}

    on = 1; 
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    if (setnonblock(listen_fd) < 0) 
    {
        fprintf(stderr, "Failed to set to non-blocking\n");
        close(listen_fd);
		return -1;
	}
     
     // REGISTER EVENTS FOR IPV4 SOCKETS
	event_assign(&serverEvent, base, listen_fd, EV_READ | EV_PERSIST, ServerAccept, NULL);

    /******************************
    * FOR IPV6 ADDRESSES
    ******************************/
#ifdef IPV6_V6ONLY

    ip6_fd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if(ip6_fd == -1)
    {
        perror("socket()");
        return -1;
    }

    memset (&ip6_addr, 0, sizeof(ip6_addr));

    ip6_addr.sin6_family = AF_INET6;
    ip6_addr.sin6_addr = in6addr_any;
    ip6_addr.sin6_port = htons(port);

    if(setsockopt(ip6_fd, SOL_SOCKET, SO_REUSEADDR, &on6, sizeof(on6)) < 0)
    {
        perror("setsockopt()");
        return -1;
    }

    if(bind(ip6_fd, (struct sockaddr *)&ip6_addr, sizeof(ip6_addr)) < 0)
    {
        perror("bind()");
        close(ip6_fd);
        return -1;
    }

    if(listen(ip6_fd, 5) < 0)
    {
        perror("listen()");
        close(ip6_fd);
        return -1;
    }

    if (setnonblock(ip6_fd) < 0) 
    {
        fprintf(stderr, "Failed to set to non-blocking\n");
        close(ip6_fd);
		return -1;
	}

    // REGISTER EVENTS FOR IPV6 SOCKETS
    event_assign(&serverEvent, base, ip6_fd, EV_READ | EV_PERSIST, ServerAccept, NULL);

#endif


	//Add event to the event set that libevent listens to
    int a = event_add(&serverEvent, NULL);
	if (a < 0) 
    {
		fprintf(stderr, "event_add(): can not add accept event into libevent");
		close(listen_fd);
#ifdef IPV6_V6ONLY
        close(ip6_fd);
#endif
		return -1;
	}

    event_base_dispatch(base);
    
    player_del(p);

    return 0;
}

/***********************************
* END OF MAIN
***********************************/

 //Set a socket to non-blocking mode.
 
int setnonblock(int fd)
{
	int flags;

	flags = fcntl(fd, F_GETFL);
	if (flags < 0)
		return flags;

	flags |= O_NONBLOCK;
	if (fcntl(fd, F_SETFL, flags) < 0)
		return -1;

	return 0;
}


//This function is called by libevent when the client's socket is readable
void ServerRead(int fd, short ev, void * arg)
{
	char buf [256];
	int len;

	len = read(fd, buf, sizeof(buf));
	if(len == 0) 
    {
		/* The client disconnects, remove the read event here and release the client data structure */
		fprintf(stderr, "disconnected\n");
		close(fd);
		event_del(&readEvent);
		return;
	} 

    else if (len <0) 
    {
		// There are other errors
		fprintf(stderr, "socket fail %s\n", strerror(errno));
		close(fd);
		event_del(&readEvent);
		return;
	}

    int res = player_post_challenge(p, buf, &msg);
    if(res > 0)
    {
        event_add(&writeEvent, NULL); 
        (void) free(msg);  
    }
	
    return;
}



//This function is called by libevent when the client's socket is writable
void ServerWrite (int fd, short ev, void * arg)
{
	if (! arg)
	{
		printf ("ServerWrite err! arg null\n");
		return;
	}
    
    char *data = arg;
    int len = strlen(data);
	if (len <= 0)
	{
		printf ("ServerWrite err! len: %d\n", len);
		return;
	}	

    int wlen = write(fd, data, len);
    if (wlen < len) 
    {
        printf ("not all data write back to client! written: %d expected: %d\n", wlen, len);
        return;
    }	
}


void ServerAccept (int fd, short ev, void * arg)
{
	int client_fd, rc;
	struct sockaddr_in addr;
	socklen_t addrlen = sizeof(addr);
	
	client_fd = accept (fd, (struct sockaddr *) &addr, & addrlen);
	if (client_fd < 0) 
    {
		fprintf (stderr, "accept(): can not accept client connection");
		return;
	}

	if (setnonblock(client_fd) < 0) 
    {
        fprintf(stderr, "Failed to set to non-blocking\n");
		close (client_fd);
		return;
	}
    

    event_assign(&readEvent, base, client_fd, EV_READ | EV_PERSIST, ServerRead, NULL);
	event_assign(&writeEvent, base, client_fd, EV_WRITE, ServerWrite, msg);

    //add game play here
    rc = player_get_greeting(p, &msg);
    if (rc > 0) 
    {
        event_add(&writeEvent, NULL);
        (void) free(msg);
    }

    while (! (p->state & PLAYER_STATE_FINISHED)) 
    {
        p->state = PLAYER_STATE_CONTINUE;
        if (! (p->state & PLAYER_STATE_CONTINUE)) 
        {
            player_fetch_chlng(p);
        }

        rc = player_get_challenge(p, &msg);
        if (rc > 0) 
        {
            event_add(&writeEvent, NULL);
            (void) free(msg);
        }

        event_add(&readEvent, NULL);
    }

    if(p->state & PLAYER_STATE_FINISHED)
    {
        event_del(&readEvent);
        event_del(&writeEvent);
    }
    
    return;
}


