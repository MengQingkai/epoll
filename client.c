#include <netinet/in.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <arpa/inet.h>

#define MAXSIZE 1024000
#define FDSIZE        1024
#define EPOLLEVENTS (1024*3)


int acksize=4;
char ipaddress[20];
int port;
char MSG[MAXSIZE];

static void handle_connection(int sockfd);
static void handle_events(int epollfd,struct epoll_event *events,int num,int sockfd,char *buf);
static void do_read(int epollfd,int fd,int sockfd,char *buf);
static void do_read(int epollfd,int fd,int sockfd,char *buf);
static void do_write(int epollfd,int fd,int sockfd,char *buf);
static void add_event(int epollfd,int fd,int state);
static void delete_event(int epollfd,int fd,int state);
static void modify_event(int epollfd,int fd,int state);

void init_msg()
{
	int i = 0;
	MSG[0] = 'A';
	for (i = 1; i < MAXSIZE; i++) 
		MSG[i] = 'B';
	MSG[i] = 0;
}

int main(int argc,char *argv[])
{
    int sockfd;
	int ret;
	if (argc < 3 ){
		printf("Format:  ./client server_ip server_port\n");
		return 0;
	}
	sscanf(argv[1], "%s", ipaddress);
	sscanf(argv[2], "%d", &port);
	init_msg();

    struct sockaddr_in  servaddr;
    sockfd = socket(AF_INET,SOCK_STREAM,0);
    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    inet_pton(AF_INET,ipaddress,&servaddr.sin_addr);
    ret = connect(sockfd,(struct sockaddr*)&servaddr,sizeof(servaddr));
    if (ret < 0) {
        perror("connect failed!\n");
        exit(-1);
    }
    //处理连接
    handle_connection(sockfd);
    close(sockfd);
    return 0;
}


static void handle_connection(int sockfd)
{
    int epollfd;
    struct epoll_event events[EPOLLEVENTS];
    char buf[acksize];
    int ret;
    epollfd = epoll_create(FDSIZE);
    add_event(epollfd,sockfd,EPOLLOUT);
    for ( ; ; )
    {
        ret = epoll_wait(epollfd,events,EPOLLEVENTS,-1);
        handle_events(epollfd,events,ret,sockfd,buf);
    }
    close(epollfd);
}

static void handle_events(int epollfd,struct epoll_event *events,int num,int sockfd,char *buf)
{
    int fd;
    int i;
    for (i = 0;i < num;i++)
    {   
        fd = events[i].data.fd;
        if (events[i].events & EPOLLIN)
            do_read(epollfd,fd,sockfd,buf);
        else if (events[i].events & EPOLLOUT)
            do_write(epollfd,fd,sockfd,MSG);
    }
}

static void do_read(int epollfd,int fd,int sockfd,char *buf)
{
    int nread;
    nread = read(fd,buf,acksize);// ACK from server to client
     if (nread == -1)
    {
        perror("read error:");
        close(fd);
    }
    else if (nread == 0)
    {
        fprintf(stderr,"server close.\n");
        close(fd);
    }
    else
    {
        printf("ACK:%s\n",buf);
        if (fd == STDIN_FILENO)
            add_event(epollfd,sockfd,EPOLLOUT);
        else
        {
            delete_event(epollfd,sockfd,EPOLLIN);
            add_event(epollfd,sockfd,EPOLLOUT);
        }
    }
    memset(buf,0,acksize);
}

static void do_write(int epollfd,int fd,int sockfd,char *buf)
{
    int nwrite,sum=0,i=1;
    
   // while(sum<MAXSIZE)
   //{
   //	nwrite=write(fd,buf+sum,strlen(buf+sum));
   //	sum+=nwrite;
   //	printf("the %d time(s) :%d   total:%d\n",i++,nwrite,sum );
   //}
   //printf("To server: %d\n", sum);

    while(sum!=MAXSIZE)
    {
    	nwrite=write(fd,buf,strlen(buf));
    	sum+=nwrite;
    	printf("the %d time(s) :%d   total:%d\n",i++,nwrite,sum );
    }
   
	if (nwrite == -1)
    {
        perror("write error:");
        close(fd);
    }
    else
    {
        if (fd == STDOUT_FILENO)
            delete_event(epollfd,fd,EPOLLOUT);
        else
            modify_event(epollfd,fd,EPOLLIN);
    }
}

static void add_event(int epollfd,int fd,int state)
{
    struct epoll_event ev;
    ev.events = state;
    ev.data.fd = fd;
    epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&ev);
}

static void delete_event(int epollfd,int fd,int state)
{
    struct epoll_event ev;
    ev.events = state;
    ev.data.fd = fd;
    epoll_ctl(epollfd,EPOLL_CTL_DEL,fd,&ev);
}

static void modify_event(int epollfd,int fd,int state)
{
    struct epoll_event ev;
    ev.events = state;
    ev.data.fd = fd;
    epoll_ctl(epollfd,EPOLL_CTL_MOD,fd,&ev);
}
