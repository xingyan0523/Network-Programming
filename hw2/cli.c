#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>

#define SERV_PORT 8080
#define MAXLINE 4096

int sockfd;
char signal_buffer[64];

int max(int a, int b){
	if(a > b)	return a;
	return b;
}

void sig_logout(int signal){
	write(sockfd, "1\n", 3);
	fputs("\n", stdout);
	exit(0);
}

void sig_cont(int signal){
	char buffer[128] = "";
	strcat(buffer, "6\n");
	strcat(buffer, signal_buffer);
	write(sockfd, buffer, strlen(buffer));
}

void game_cli(FILE *fp, int sockfd) {
	char sendline[MAXLINE], recvline[MAXLINE];
	char name[64] = "";
	int state;
	char buffer[128];
	char input[4096];
login:	
	sprintf(sendline, "0\n");
	printf("username: ");
	scanf("%s", buffer);
	strcat(sendline, buffer);
	strcat(name, buffer);
	strcat(sendline, "\n");
	printf("password: ");
	scanf("%s", buffer);
	strcat(sendline, buffer);
	strcat(sendline, "\n");
	write(sockfd, sendline, (int)strlen(sendline));
	read(sockfd, recvline, MAXLINE);
	state = recvline[0] - '0';
	fputs(recvline+2, stdout);

	if(state) goto login;
			
	signal(SIGINT, sig_logout);
	printf("list\t\t\t--list all online user\nselect user_name\t--send request to (user_name)\nlook\t\t\t--list all game request\naccept (user_name)\t--accept game\n");
start:
	while(fgets(input, 4096, stdin)){
		memset(recvline, 0, strlen(recvline));
		memset(sendline, 0, strlen(sendline));
		if(!strncmp(input, "logout", 6)){
			write(sockfd, "1\n", 3);
			goto login;
		}
		else if(!strncmp(input, "list", 4)){
			sprintf(sendline, "2\n");
			write(sockfd, sendline, (int)strlen(sendline));
			read(sockfd, recvline, MAXLINE);
			fputs(recvline, stdout);
		}
		else if(!strncmp(input, "select", 6)){
			sprintf(sendline, "3\n");
			char *ptr = strchr(input, '\n');
			*ptr = '\0';
			char rec_name[64];
			strcpy(rec_name, input + 7);
			strcpy(signal_buffer, rec_name);
			strcat(sendline, rec_name);
			if(!strcmp(name, rec_name)){
				printf("Wrong user\n");
				continue;
			}
			write(sockfd, sendline, (int)strlen(sendline));
			read(sockfd, recvline, MAXLINE);
			fputs(recvline+2, stdout);
			//if(strncmp(recvline, "4", 1)) continue;
			//signal(SIGINT, sig_cont);
			memset(recvline, 0, strlen(recvline));
			memset(sendline, 0, strlen(sendline));

			read(sockfd, recvline, MAXLINE);
			fputs(recvline+2, stdout);
			while(strncmp(recvline, "88", 2)){
				int x, y;
				int table_id;
					
				while(1){
					memset(recvline, 0, strlen(recvline));
					memset(sendline, 0, strlen(sendline));
					scanf("%d%d", &x, &y);
					sprintf(sendline, "7\n%d\n%d\n%d\n", x, y, table_id);
					write(sockfd, sendline, (int)strlen(sendline));
					read(sockfd, recvline, MAXLINE);
					if(!strncmp(recvline, "7", 1)){
						memset(recvline, 0, strlen(recvline));
						break;
					}
					else{
						printf("Wrong coord!\nPlease re-enter the coord(x y): \n");
					}
				}
				sendline[0] = '8';
				write(sockfd, sendline, (int)strlen(sendline));
				read(sockfd, recvline, MAXLINE);
				if(!strncmp(recvline, "8", 1)){
					fputs(recvline+2, stdout);
				}
				else if(!strncmp(recvline, "9", 1)){
					fputs(recvline+2, stdout);
					read(sockfd, recvline, MAXLINE);
					fputs(recvline+2, stdout);
				}
			}
				//signal(SIGINT, sig_logout);
		}
		else if(!strncmp(input, "look", 4)){
			sprintf(sendline, "4\n");
			write(sockfd, sendline, (int)strlen(sendline));
			read(sockfd, recvline, MAXLINE);
			fputs(recvline, stdout);
		}
		else if(!strncmp(input, "accept", 6)){
			sprintf(sendline, "5\n");
			char *ptr = strchr(input, '\n');
			*ptr = '\0';
			strcat(sendline, input + 7);
			if(!strcmp(name, input+7)){
				printf("Wrong user\n");
				continue;
			}
			write(sockfd, sendline, (int)strlen(sendline));
			read(sockfd, recvline, MAXLINE);
			fputs(recvline+2, stdout);

			read(sockfd, recvline, MAXLINE);
			fputs(recvline+2, stdout);
			// read(sockfd, recvline, MAXLINE);
			// fputs(recvline, stdout);
			while(strncmp(recvline, "88", 2)){
				int x, y;
				int table_id;
					
				while(1){
					memset(recvline, 0, strlen(recvline));
					memset(sendline, 0, strlen(sendline));
					scanf("%d%d", &x, &y);
					sprintf(sendline, "7\n%d\n%d\n%d\n", x, y, table_id);
					write(sockfd, sendline, (int)strlen(sendline));
					read(sockfd, recvline, MAXLINE);
					if(!strncmp(recvline, "7", 1)){
						memset(recvline, 0, strlen(recvline));
						break;
					}
					else{
						printf("Wrong coord!\nPlease re-enter the coord(x y): \n");
					}
				}
				sendline[0] = '8';
				write(sockfd, sendline, (int)strlen(sendline));
				read(sockfd, recvline, MAXLINE);
						
				if(!strncmp(recvline, "8", 1)){
					fputs(recvline+2, stdout);
				}
				else if(!strncmp(recvline, "9", 1)){
					fputs(recvline+2, stdout);
					read(sockfd, recvline, MAXLINE);
					fputs(recvline+2, stdout);
				}
			}				
		}
		else if(!strncmp(input, "\n", 1)){

		}
		else{
			printf("error\n");
		}
	}
}


int main(int argc, char **argv) {
	int	i;
	struct sockaddr_in	servaddr;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(SERV_PORT);
		
	connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr));

	game_cli(stdin, sockfd);		/* do it all */

	exit(0);
}