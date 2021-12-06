#include <stdbool.h>

#define SERV_PORT 8080
#define MAXLINE 4096
#define PLAYER 2
#define SETSIZE 64

struct table;

int find_empty_table(struct table *tb);

bool check_axis(int x, int y, struct table *tb, int table_id);

int parse(struct table *tb, int player, int table_id);

bool draw(struct table *tb, int table_id);

void ascii(char *buf, int table[3][3]);