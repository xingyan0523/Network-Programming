#include <stdio.h>
#include <stdbool.h>
#include "tic_tac_toe.h"

struct table{
	bool stat[64];
	int v[64][3][3];
};

int find_empty_table(struct table *tb){
	for(int i = 0; i < SETSIZE; i++){
		if(!tb->stat[i]){
			tb->stat[i] = true;
			return i;
		}
	}
}

int reset_game(struct table *tb, int table_id){

}

bool check_axis(int x, int y, struct table *tb, int table_id){
    if(x < 0 || x > 2 || y < 0 || y > 2) return false;
    if(tb->v[table_id][x][y] != -1) return false;
    return true;
}

int parse(struct table *tb, int player, int table_id){
    printf("*0*\n");
    
    for(int x = 0; x < 3; x++){
        int i = 0;
        for(int y = 0; y < 3; y++){
            if(tb->v[table_id][x][y] == player) i++;
            else break;
		}
        if(i == 3) return player;
    }
    printf("*1*\n");

    for(int x = 0; x < 3; x++){
        int i = 0;
        for(int y = 0; y < 3; y++){
            if(tb->v[table_id][y][x] == player) i++;
            else break;
		}
        if(i == 3) return player;
    }
    
    printf("*2*\n");
    int i = 0;
    for(int x = 0; x < 3; x++){
        if(tb->v[table_id][x][x] == player) i++;
        else break;
    }
    if(i == 3) return player;
    printf("*3*\n");
    i = 0;
    int y = 2;
    for(int x = 0; x < 3; x++, y--){
        if(tb->v[table_id][x][y] == player) i++;
        else break;
    }
    if(i == 3) return player;
    
    return -1;
}

bool draw(struct table *tb, int table_id){
    for(int i = 0; i < 3; i++){
        for(int j = 0; j < 3; j++){
            if(tb->v[table_id][i][j] == -1) return false;
        }
    }
    return true;
}

void ascii(char *buf, int table[3][3]){
    int count = 0;
    count += sprintf(buf, "-------\n");
    
    for(int i = 0; i < 3; i++){
        count += sprintf(buf + count, "|");
        
        for(int j = 0; j < 3; j++){
            if(table[i][j] == 0){
                count += sprintf(buf + count, "O");
            }
            else if(table[i][j] == 1){
                count += sprintf(buf + count, "X");
            }
            else count += sprintf(buf + count, " ");
            count += sprintf(buf + count, "|");
        }
        count += sprintf(buf + count, "\n");
    }
    count += sprintf(buf + count, "-------\n");
}