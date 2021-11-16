#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>



#define SERV_PORT 8080
#define BUFSIZE 8192
char packet[BUFSIZE];

void sig_chld(int sig){
	while(waitpid(-1, 0, 0) > 0);
}


char* memstr(char* full_data, int full_data_len, char* substr)
{
    if (full_data == NULL || full_data_len <= 0 || substr == NULL) {
        return NULL;
    }
 
    if (*substr == '\0') {
        return NULL;
    }
 
    int sublen = strlen(substr);
 
    int i;
    char* cur = full_data;
    int last_possible = full_data_len - sublen + 1;
    for (i = 0; i < last_possible; i++) {
        if (*cur == *substr){
            if (memcmp(cur, substr, sublen) == 0) {
                return cur;
            }
        }
        cur++;
    }
 
    return NULL;
}

const char *get_content_type(const char* path) {
    const char *last_dot = strrchr(path, '.');
    if (last_dot) {
        if (strcmp(last_dot, ".css") == 0) return "text/css";
        if (strcmp(last_dot, ".csv") == 0) return "text/csv";
        if (strcmp(last_dot, ".gif") == 0) return "image/gif";
        if (strcmp(last_dot, ".htm") == 0) return "text/html";
        if (strcmp(last_dot, ".html") == 0) return "text/html";
        if (strcmp(last_dot, ".ico") == 0) return "image/x-icon";
        if (strcmp(last_dot, ".jpeg") == 0) return "image/jpeg";
        if (strcmp(last_dot, ".jpg") == 0) return "image/jpeg";
        if (strcmp(last_dot, ".js") == 0) return "application/javascript";
        if (strcmp(last_dot, ".json") == 0) return "application/json";
        if (strcmp(last_dot, ".png") == 0) return "image/png";
        if (strcmp(last_dot, ".pdf") == 0) return "application/pdf";
        if (strcmp(last_dot, ".svg") == 0) return "image/svg+xml";
        if (strcmp(last_dot, ".txt") == 0) return "text/plain";
    }

    return "application/octet-stream";
}

void handler(int fd) {
	int file_fd, buflen, len;
	long i, ret;
	static char buffer[BUFSIZE + 1];

	ret = read(fd, buffer, BUFSIZE);
	// printf("in:\n%s\n", buffer);

	if(ret < BUFSIZE) buffer[ret] = 0;


	if(!strncmp(buffer, "GET / ", 6))
		strcpy(buffer, "GET /public/index.html\0");
	if(!strncmp(buffer, "POST", 4)) {
		buflen = strlen(buffer);
		buffer[buflen] = '\0';
		ret = read(fd, buffer, BUFSIZE);

		char filename[128];
		char path[128] = "./upload/";
		char *ptr = strstr(buffer, "filename=\"");
		ptr += 10;
		char *end = strchr(ptr, '\"');

		strncpy(filename, ptr, end-ptr);
		filename[end-ptr] ='\0';
		strcat(path, filename);

		FILE *fp, *log;
		fp = fopen(path, "wb");
		log = fopen("log", "a");

		ptr = strstr(buffer, "Content-Type:");
		ptr = strchr(ptr, '\n');
		ptr += 3;
		buflen = ret - (uint8_t)(ptr-buffer);
		
		end = memstr(ptr, buflen, "\r\n------WebKit");

		if(end){ //find boundary
			fwrite(ptr, end - ptr, 1, fp);
		}
		else{
			fwrite(ptr, ret - (uint8_t)(ptr-buffer), 1, fp);
			while(1){
				buflen = read(fd, buffer, BUFSIZE);
				ptr = buffer;
		
				end = memstr(ptr, buflen, "\r\n------WebKit");
				if(end){
					fwrite(ptr, end - ptr, 1, fp);
					break;
				}
				fwrite(ptr, buflen, 1, fp);
			}
		}
		/* -----update log----- */
		time_t rawtime;
		struct tm *timeinfo;
		time ( &rawtime );
		timeinfo = localtime ( &rawtime );
		fprintf (log, "filename: %s\ttime: %s", filename, asctime (timeinfo));
		/* -------------------- */

		strcpy(buffer, "GET /public/index.html\0");
	}
	if(!strncmp(buffer, "GET /", 5)) {
		for(i = 4; i < BUFSIZE; i++) {
			if(buffer[i] == ' ') {
				buffer[i] = 0;
				break;
			}
		}

		if((file_fd = open(&buffer[5],O_RDONLY)) < 0)
			write(fd, "Failed to open file", 19);

		sprintf(buffer, "HTTP/1.0 200 OK\r\nContent-Type: %s\r\n\r\n", get_content_type(buffer));
		write(fd, buffer, strlen(buffer));
		//printf("ret:\n%s", buffer);
		while((ret = read(file_fd, buffer, BUFSIZE)) > 0) {
			write(fd, buffer, ret);
		}

	}
	close(file_fd);
	close(fd);
}



int main(int argc, char **argv) {
	int					listenfd, connfd;
	pid_t				childpid;
	socklen_t			clilen;
	struct sockaddr_in	cliaddr, servaddr;
	
	listenfd = socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(SERV_PORT);

	bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));

	listen(listenfd, SOMAXCONN);
	signal(SIGCHLD, sig_chld);
	
	while(1) {
		clilen = sizeof(cliaddr);
		if ( (connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &clilen)) < 0) {
			if (errno == EINTR)
				continue;		/* back to for() */
			else
				printf("accept error");
		}
		if ( (childpid = fork()) == 0) {	/* child process */
			close(listenfd);	/* close listening socket */
			handler(connfd);	/* process the request */
			//printf("--%s\n", packet);
			exit(0);
		}
		close(connfd);			/* parent closes connected socket */
	}
}