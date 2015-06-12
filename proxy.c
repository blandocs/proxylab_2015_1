/*
 * proxy.c - CS:APP Web proxy
 *
 * Student ID:2013-11393
 *         Name:KimHyunSu
 * 
 * IMPORTANT: Give a high level description of your code here. You
 * must also provide a header comment at the beginning of each
 * function that describes what that function does.
 */ 

#include "csapp.h"
#include "string.h"

/* The name of the proxy's log file */
#define PROXY_LOG "proxy.log"

/* Undefine this if you don't want debugging output */
#define DEBUG

/*
 * Functions to define
 */
void *process_request(void* vargp);
int open_clientfd_ts(char *hostname, int port, sem_t *mutexp);
ssize_t Rio_readn_w(int fd, void *ptr, size_t nbytes);
ssize_t Rio_readlineb_w(rio_t *rp, void *usrbuf, size_t maxlen);
void Rio_writen_w(int fd, void *usrbuf, size_t n);

//semaphore
sem_t mutex;
char *client_ip;
int c_port;
int bytecount;
/*
 * main - Main routine for the proxy program
 */
int main(int argc, char **argv)
{
	//semaphore initialize
	Sem_init(&mutex,0,1);

    /* Check arguments */
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <port number>\n", argv[0]);
    exit(0);
  }

	int port = atoi(argv[1]);
	struct sockaddr_in clientaddr;
	int clientlen=sizeof(clientaddr);
	pthread_t tid;

	int listenfd = Open_listenfd(port);
 
	while (1) {
		int *connfdp = Malloc(sizeof(int));
		struct hostent *hp;
		char *haddrp;
		int client_port;

		*connfdp = Accept(listenfd, (SA *) &clientaddr, &clientlen);
		/* determine the domain name and IP address of the client */
		hp = Gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, sizeof(clientaddr.sin_addr.s_addr), AF_INET);
		haddrp = inet_ntoa(clientaddr.sin_addr);
		
		client_ip = haddrp;
		
		client_port = ntohs(clientaddr.sin_port);

		c_port = client_port;

		printf("Server connected to %s (%s), port %d\n",
		hp->h_name, haddrp, client_port);
		Pthread_create(&tid, NULL, process_request, connfdp);

		
	}

	 
   exit(0);
}

void *process_request(void* vargp){

	char prefix[40];
	pthread_t tid = pthread_self();
	int connfd = *((int *)vargp);
	Pthread_detach(pthread_self());
	Free(vargp);

	printf("Served by thread %d\n", (int) tid);
	sprintf(prefix, "Thread %d ", (int) tid);

	size_t n;
	char buf[MAXLINE];
	rio_t rio;

	Rio_readinitb(&rio, connfd);

	//for split input

	char *input;
	char hostname[MAXLINE];
	char port[MAXLINE];
	char message[MAXLINE];
	
	bytecount = 0;

	while((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
		//split command
		input = buf;
		printf("buf: %s \n",buf);
		if(strchr(input,' ') != NULL){
			input = strchr(input, ' ') + 1;
			if(*input != '\0' && strchr(input,' ')!= NULL){
				input = strchr(input, ' ') + 1;
				if(*input != '\0')
					printf("good input\n");
				else{
			strcpy(buf, "proxy usage: <host> <port> <message>\n");
			Rio_writen(connfd, buf, strlen(buf));
			continue;

				}
			}
			else{
			strcpy(buf, "proxy usage: <host> <port> <message>\n");
			Rio_writen(connfd, buf, strlen(buf));
			continue;

			}

		}else{
			strcpy(buf, "proxy usage: <host> <port> <message>\n");
			Rio_writen(connfd, buf, strlen(buf));
			continue;

		}
		//split finish and check the wrong input
		
		//get arguments from client
		input = buf;
	
		strncpy(hostname,input,strlen(input) - strlen(strchr(input,' ')));

		input = strchr(input, ' ');
	
		input += 1;
		strncpy(port,input,strlen(input) - strlen(strchr(input,' ')));
	
		input = strchr(input, ' ');
	
		strcpy(message,input + 1);

		printf("%sreceived %d bytes \n", prefix, (int) n);
		
		printf("%s %s %s\n",hostname,port,message);

		//connect to server
		rio_t rio2;
		int clientfd;

		clientfd = open_clientfd_ts(hostname,atoi(port), &mutex);
		Rio_readinitb(&rio2, clientfd);

		Rio_writen(clientfd, message, strlen(message));
		n =	Rio_readlineb(&rio2, message, MAXLINE);
		bytecount += n; 
		Rio_writen(connfd, message, n);


		//Make log file
		FILE* fd;
		fd = Fopen("proxy.log","a");
		
		//get current time
		char answer[MAXLINE];
		time_t timer;
		struct tm *t;
		char *day;
		char *month;
		timer = time(NULL);
		t = localtime(&timer);
		switch((t->tm_wday) +1){
		case 1:
			day = "MON";
			break;
		case 2:
			day = "TUE";
			break;
		case 3:
			day = "WED";
			break;
		case 4:
			day = "THR";
			break;
		case 5:
			day = "FRI";
			break;
		case 6:
			day = "SAT";
			break;
		case 7:
			day = "SUN";
		}

		switch((t->tm_mon)+1){
		case 1:
			month = "JAN";
			break;
		case 2:
			month = "FEB";
			break;
		case 3:
			month = "MAR";
			break;
		case 4:
			month = "APR";
			break;
		case 5:
			month = "MAY";
			break;
		case 6:
			month = "JUN";
			break;
		case 7:
			month = "JUL";
			break;
		case 8:
			month = "AUG";
			break;
		case 9:
			month = "SEP";
			break;
		case 10:
			month = "OCT";
			break;
		case 11:
			month = "NOV";
			break;
		case 12:
			month = "DEC";
		}

		sprintf(answer, "%s %d %s %d %d:%d:%d KST: %s %d %d %s",day,t->tm_mday,month,t->tm_year+1900,t->tm_hour,t->tm_min,t->tm_sec,client_ip,c_port,bytecount, message);

		Fwrite(answer,1,strlen(answer),fd);
		Fclose(fd);

		Close(clientfd);
		
	}
	Close(connfd);
	return NULL;
}

int open_clientfd_ts(char *hostname, int port, sem_t *mutexp){
  int clientfd;
  struct hostent *hp;
  struct sockaddr_in serveraddr;
	
	if ((clientfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		return -1; /* check errno for cause of error */

  /* Fill in the server's IP address and port */
  if ((hp = gethostbyname(hostname)) == NULL)
		return -2; /* check h_errno for cause of error */
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  bcopy((char *)hp->h_addr_list[0],
  (char *)&serveraddr.sin_addr.s_addr, hp->h_length);
  serveraddr.sin_port = htons(port);

  /* Establish a connection with the server */
	if (connect(clientfd, (SA *) &serveraddr, sizeof(serveraddr)) < 0)
		return -1;
  return clientfd;
}

ssize_t Rio_readn_w(int fd, void *ptr, size_t nbytes){
	ssize_t n;
	
	if ((n = rio_readn(fd, ptr, nbytes)) < 0)
		printf("Rio_readn_w error\n");
	return n;
}

ssize_t Rio_readlineb_w(rio_t *rp, void *usrbuf, size_t maxlen){
	ssize_t rc;
	
	if ((rc = rio_readlineb(rp, usrbuf, maxlen)) < 0)
		printf("Rio_readlineb_w error\n");
	return rc;
}

void Rio_writen_w(int fd, void *usrbuf, size_t n){
	if (rio_writen(fd, usrbuf, n) != n)
		printf("Rio_writen_w error\n");
}

