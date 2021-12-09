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
#include <stdbool.h>
#include "tic_tac_toe.h"

#define SERV_PORT 8080
#define MAXLINE 4096
#define PLAYER 2
#define SETSIZE 64

char name[2][32];
char password[2][32];
int fd_arr[SETSIZE];
bool online[SETSIZE];
int req[SETSIZE][SETSIZE];

struct table{
	bool stat[SETSIZE];
	int v[SETSIZE][3][3];
	int fd[SETSIZE][2];
};

struct rank{
	int player;
	int win;
	int lose;
	int draw;
	int score;
}r[SETSIZE];

bool login(char *n, char *p, int fd){
	for(int i = 0; i < PLAYER; i++){
		if(!strcmp(name[i], n)){
			if(!strcmp(password[i], p)){
				online[i] = true;
				fd_arr[i] = fd;
				return true;
			}
			break;
		}
	}
	return false;
}

void logout(int fd){
	for(int i = 0; i < PLAYER; i++){
		if(fd_arr[i] == fd){
			fd_arr[i] = -1;
			for(int j = 0; j < PLAYER; j++)
				req[i][j] = 0;
			online[i] = false;
			break;
		}
	}
}

void list_user(char *buf){
	int count = 0;
	for(int i = 0; i < PLAYER; i++){
		if(online[i]){
			count = sprintf(buf + count, "%s\n", name[i]);
		}
	}
}

int req_user(char *n, int fd, int *arr){
	int req_id, rec_id;
	int ret = 0;
	for(int i = 0; i < PLAYER; i++){
		if(!strcmp(name[i], n)){
			rec_id = i;
			ret = 1;
		}
		if(fd_arr[i] == fd){
			req_id = i;
		}
	}
	if(ret){
		req[req_id][rec_id] = 1;
		arr[0] = fd_arr[req_id];
		arr[1] = fd_arr[rec_id];
	} 
	if(req[rec_id][req_id]) return 2;
	return ret;
}

void req_cancel(char *n, int fd){
	int req_id, rec_id;
	int ret = 0;
	for(int i = 0; i < PLAYER; i++){
		if(!strcmp(name[i], n)){
			rec_id = i;
			ret = 1;
		}
		if(fd_arr[i] == fd){
			req_id = i;
		}
	}
	req[req_id][rec_id] = 0;
}

void look_req(char *buf, int fd){
	int count = 0;
	for(int i = 0; i < PLAYER; i++){
		for(int j = 0; j < PLAYER; j++){
			if(req[i][j]){
				count = sprintf(buf + count, "%s\n", name[i]);
			}
		}
	}
}

void init(){
	strcpy(name[0], "a");
	strcpy(name[1], "b");
	strcpy(password[0], "aa");
	strcpy(password[1], "bb");
}

void end_game(struct table *tb, int table_id){
	req[tb->fd[table_id][0]][tb->fd[table_id][1]] = 0;
	req[tb->fd[table_id][1]][tb->fd[table_id][0]] = 0;
	tb->fd[table_id][0] = tb->fd[table_id][1] = -1;
	tb->stat[table_id] = false;
	for(int i = 0; i < 3; i++){
		for(int j = 0; j < 3; j++){
			tb->v[table_id][i][j] = -1;
		}	
	}
}

int compare (const void * a, const void * b)
{

  struct rank *rka = (struct rank *)a;
  struct rank *rkb = (struct rank *)b;

  return ( rkb->score - rka->score);
}

void rank_req(char *buf){
	qsort(r, PLAYER, sizeof(r[0]), compare);
	int count = 0;
	count += sprintf(buf, "-------------------rank-------------------\n");
	count += sprintf(buf + count, "no.\tname\twin\tdraw\tlose\tscore\n");
	for(int i = 0; i < PLAYER; i++){
		count += sprintf(buf + count, "%d\t%s\t%d\t%d\t%d\t%d\n", i+1, name[r[i].player], r[i].win, r[i].draw, r[i].lose, r[i].score);
	}
}

int main(int argc, char **argv){
	int					i, maxi, max_fd, litsen_fd, conn_fd, sockfd;
	int					n_ready, client[SETSIZE];
	ssize_t				n;
	fd_set				rset, allset;
	char				buf[MAXLINE];
	socklen_t			clilen;
	struct sockaddr_in	cliaddr, servaddr;
	struct table tb;
	struct rank rk;

	init();

	litsen_fd = socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(SERV_PORT);

	bind(litsen_fd, (struct sockaddr *) &servaddr, sizeof(servaddr));

	/* table init */
	for(i = 0; i < SETSIZE; i++){
		fd_arr[i] = tb.fd[i][0] = tb.fd[i][1] = -1;
		for(int j = 0; j < PLAYER; j++)
			req[i][j] = 0;
		online[i] = false;
		tb.stat[i] = false;
		for(int x = 0; x < 3; x++){
			for(int y = 0; y < 3; y++){
		    	tb.v[i][x][y] = -1;
	    	}
		}
		r[i].player = i;
		r[i].win = r[i].lose = r[i].draw = r[i].score = 0;
	}


	listen(litsen_fd, SOMAXCONN);

	max_fd = litsen_fd;			/* initialize */
	maxi = -1;					/* index into client[] array */
	for (i = 0; i < SETSIZE; i++)
		client[i] = -1;			/* -1 indicates available entry */
	FD_ZERO(&allset);
	FD_SET(litsen_fd, &allset);

/* include fig02 */
	while(1) {
		rset = allset;		/* structure assignment */
		n_ready = select(max_fd+1, &rset, NULL, NULL, NULL);

		if (FD_ISSET(litsen_fd, &rset)) {	/* new client connection */
			clilen = sizeof(cliaddr);
			conn_fd = accept(litsen_fd, (struct sockaddr *) &cliaddr, &clilen);

			for (i = 0; i < SETSIZE; i++)
				if (client[i] < 0) {
					client[i] = conn_fd;	/* save descriptor */
					break;
				}
			if (i == SETSIZE)
				printf("too many clients");

			FD_SET(conn_fd, &allset);	/* add new descriptor to set */
			if (conn_fd > max_fd)
				max_fd = conn_fd;			/* for select */
			if (i > maxi)
				maxi = i;				/* max index in client[] array */

			if (--n_ready <= 0)
				continue;				/* no more readable descriptors */
		}

		for (i = 0; i <= maxi; i++) {	/* check all clients for data */
			if ( (sockfd = client[i]) < 0)
				continue;
			if (FD_ISSET(sockfd, &rset)) {
				if ( (n = read(sockfd, buf, MAXLINE)) == 0) {
						/*4connection closed by client */
					close(sockfd);
					FD_CLR(sockfd, &allset);
					client[i] = -1;
				} else{
					int state;
					char *ptr = buf;
					state = *ptr - '0';
					char *end;
					if(state == 0){
						char na[32] = "";
						char pa[32] = "";
						ptr = strchr(ptr, '\n'); ptr++;
						end = strchr(ptr, '\n');
						strncpy(na, ptr, end-ptr);
						ptr = end + 1;
						end = strchr(ptr, '\n');
						strncpy(pa, ptr, end-ptr);
						printf("%d, %s, %s\n", state, na, pa);

						if(login(na, pa, sockfd)){
							sprintf(buf, "0\nFind a user to join game.\n");
						}
						else{
							sprintf(buf, "1\nUser name or password is wrong.\n");
						}
						write(sockfd, buf, (int)strlen(buf));
						memset(buf, 0, strlen(buf));
					}
					else if(state == 1){
						logout(sockfd);
					}
					else if(state == 2){
						list_user(buf);
						write(sockfd, buf, (int)strlen(buf));
					}
					else if(state == 3){
						int fd[2];
						if(req_user(buf+2, sockfd, fd)){
							sprintf(buf, "3\nWating for response.\n");
						}
						else{
							sprintf(buf, "4\nUser not found.\n");
						}
						write(sockfd, buf, (int)strlen(buf));
					}
					else if(state == 4){
						look_req(buf, sockfd);
						write(sockfd, buf, (int)strlen(buf));
					}
					else if(state == 5){
						int ret;
						int fd[2];
						if(ret = req_user(buf+2, sockfd, fd) == 2){
							int table_id = find_empty_table(&tb);
							sprintf(buf, "5\ngame id:%d\nGame start.\n", table_id);
							ascii(buf + (int)strlen(buf), tb.v[table_id]);
							tb.fd[table_id][0] = fd[0];
							tb.fd[table_id][1] = fd[1];
							int len = strlen(buf);
							
							strcat(buf, "Waiting...\n");
							write(fd[0], buf, (int)strlen(buf));
							strcpy(buf+len, "Your turn!\nEnter the coord(x y): \n");
							write(fd[1], buf, (int)strlen(buf));
							memset(buf, 0, strlen(buf));
							continue;
						}
						else if(ret == 1){
							sprintf(buf, "3\nWating for response.\n");
						}
						else{
							sprintf(buf, "4\nUser not found.\n");
						}
						write(sockfd, buf, (int)strlen(buf));
					}
					else if(state == 6){
						// req_cancel();
					}
					else if(state == 7){
						int table_id;
						int x, y;
						sscanf(buf, "%d%d%d%d", &state, &x, &y, &table_id);
						if(!check_axis(x, y, &tb, table_id)){
							printf("x: %d, y: %d\n",x ,y);
							sprintf(buf, "6\naxis error\n");
							write(sockfd, buf, (int)strlen(buf));
							continue;
						}
						else{
							sprintf(buf, "7\n");
							write(sockfd, buf, (int)strlen(buf));
						}
					}
					else if(state == 8){
						int table_id;
						int x, y;
						sscanf(buf, "%d%d%d%d", &state, &x, &y, &table_id);
						if(sockfd == tb.fd[table_id][0]){
							tb.v[table_id][x][y] = 0;
						}
						else if(sockfd == tb.fd[table_id][1]){
							tb.v[table_id][x][y] = 1;
						}
						char win[4096] = "88\nYou win!\n";
						char lose[4096] = "88\nYou lose!\n";
						if(parse(&tb, 0, table_id) == 0){
							printf("parse: 0 x:%d, y:%d table: %d\n",x ,y, table_id);
							ascii(win + (int)strlen(win), tb.v[table_id]);
							ascii(lose + (int)strlen(lose), tb.v[table_id]);
							write(tb.fd[table_id][0], win, (int)strlen(win));
							write(tb.fd[table_id][1], lose, (int)strlen(lose));
							for(int i = 0; i < PLAYER; i++){
								if(tb.fd[table_id][0] == fd_arr[i]){
									r[i].win++;
									r[i].score++;
								}
								if(tb.fd[table_id][1] == fd_arr[i]){
									r[i].lose++;
									r[i].score--;
								}
							}
							end_game(&tb, table_id);
						}
						else if(parse(&tb, 1, table_id) == 1){
							printf("parse: 1\n");
							ascii(win + (int)strlen(win), tb.v[table_id]);
							ascii(lose + (int)strlen(lose), tb.v[table_id]);
							write(tb.fd[table_id][1], win, (int)strlen(win));
							write(tb.fd[table_id][0], lose, (int)strlen(lose));
							for(int i = 0; i < PLAYER; i++){
								if(tb.fd[table_id][1] == fd_arr[i]){
									r[i].win++;
									r[i].score++;
								}
								if(tb.fd[table_id][0] == fd_arr[i]){
									r[i].lose++;
									r[i].score--;
								}
							}
							end_game(&tb, table_id);
						}
						else if(draw(&tb, table_id)){
							printf("parse: 2\n");
							sprintf(buf, "88\nDraw!\n");
							ascii(buf + (int)strlen(buf), tb.v[table_id]);
							write(tb.fd[table_id][1], buf, (int)strlen(buf));
							write(tb.fd[table_id][0], buf, (int)strlen(buf));
							for(int i = 0; i < PLAYER; i++){
								if(tb.fd[table_id][1] == fd_arr[i]){
									r[i].draw++;
								}
								if(tb.fd[table_id][0] == fd_arr[i]){
									r[i].draw++;
								}
							}
							end_game(&tb, table_id);
						}
						else{
							memset(buf, 0, strlen(buf));
							printf("parse: 3\n");
							int wait = 0;
							int turn = 1;
							if(sockfd == tb.fd[table_id][1]){
								turn = 0; wait = 1;
							}
							sprintf(buf, "9\n");
							ascii(buf + (int)strlen(buf), tb.v[table_id]);
							strcat(buf, "Waiting...\n");
							write(tb.fd[table_id][wait], buf, (int)strlen(buf));
							memset(buf, 0, strlen(buf));
							sprintf(buf, "8\n");
							ascii(buf + (int)strlen(buf), tb.v[table_id]);
							strcat(buf, "Your turn!\nEnter the coord(x y): \n");
							write(tb.fd[table_id][turn], buf, (int)strlen(buf));
						}
					}
					else if(state == 9){
						rank_req(buf);
						write(sockfd, buf, (int)strlen(buf));
					}

					memset(buf, 0, strlen(buf));
					//write(sockfd, buf, n);
				}
				if (--n_ready <= 0)
					break;				/* no more readable descriptors */
			}
		}
	}
}
/* end fig02 */