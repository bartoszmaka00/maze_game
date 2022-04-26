#ifndef PROCES1_2_SERVER_H
#define PROCES1_2_SERVER_H
#define M 25
#define N 52
#define GEM 10
#define MON 5
#define PLAYERS 4
#define KLIENTS 1
#include <semaphore.h>
sem_t* sem;
struct punkt{
    int x;
    int y;
};
struct map_klient{
    struct punkt curr;
    char c;
};
struct gem{
    struct punkt curr;
    char c;
};

struct dropped{
    struct punkt curr;
    char c;
    int carried;
};
struct gracz{
    int pid;
    struct punkt start;
    struct punkt curr;
    int deaths;
    int coins_found;
    int coins_brought;
    int krzak;
};
struct bestia{
    struct punkt curr;
    int key;
    sem_t bs;
    sem_t bx;
};
struct monsters{
    struct bestia mon[MON];
    char map[M][N];
    int ile;
    int connect;
    sem_t ms;
    sem_t mx;
};
struct klient{
    int connect;
    int pid;
    int key;
    int nr_klienta;
    struct map_klient mapa[5][5];
    struct gracz g;
    int round;
    int type;
    sem_t cs;
    sem_t cx;
};
struct slot{
    int gracze[4];
};
struct serwer{
    char map[M][N];
    int pid;
    struct punkt campsite;
    struct gem skarby[GEM+GEM];
    struct dropped drop[GEM+GEM];
    struct bestia monster[MON];
    struct klient* klienci[4];
    struct gracz gracze[4];
    int mon_count;
    int round_nr;
    int player_count;
};
void init_ncurses();
void ile_graczy(struct serwer *s,int *slot);
int serwer_init(struct serwer*);
void paint(struct serwer*);
void paint_player(struct serwer*s,int n);
void paint_maze(struct serwer*p);
void paint_all(struct serwer*p);
void paint_legend();
int player_move(struct serwer*s,int key,int nr);
void player_valid(struct serwer*s);
int put_player(struct gracz*g);
void add_gem(struct serwer*,char a);
void add_beast(struct serwer*);
void *beast_attack_pthread(void*);
void *player_pthread(void*);
void get_map(struct serwer*s,struct klient *k);
void get_all_map(struct monsters *mon);
void connection(struct serwer*s,struct klient *k);
void *exist_pthread(void *ptr);
void move_beast(struct serwer*s,struct monsters*mon);
#endif //PROCES1_2_SERVER_H
