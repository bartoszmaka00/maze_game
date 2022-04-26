#include <stdio.h>
#include <unistd.h>
#include <ncurses.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>
#include <semaphore.h>
#include <ctype.h>
#include <time.h>
#include <stdlib.h>
#define PLAYERS 4
struct punkt{
    int x;
    int y;
};
struct map_klient{
    struct punkt curr;
    char c;
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
struct slot{
    int gracze[4];
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
void init_ncurses();
void paint_map(struct klient*k);
int bot_move(struct klient*k);
int losuj();
int main() {//gcc -Wall -pedantic -o bot bot.c -lncurses -pthread -lrt

    init_ncurses();
    //pamiec wspoldzielona
    int fd_slot=shm_open("my_slot", O_RDWR,0777);
    if(fd_slot== -1){
        endwin();
        return printf("\nshm_open failed\n"),1;
    }
    ftruncate(fd_slot,sizeof (struct slot));
    int*slot=mmap(NULL, sizeof(int)*PLAYERS,PROT_READ | PROT_WRITE,MAP_SHARED,fd_slot,0);
    if(slot==MAP_FAILED){
        endwin();
        return printf("\nmmap failed\n"),1;
    }
    int nr_klienta=-1;
    for(int i=0;i<PLAYERS;i++){
        if(slot[i]==0) {
            slot[i] = 1;
            nr_klienta = i + 1;
            break;
        }
    }
    if(nr_klienta==-1){
        munmap(slot, sizeof(int)*PLAYERS);
        close(fd_slot);
        endwin();
        return printf("\nserver pelny\n"),1;
    }
    char shm_txt[4][20]={"shm_klient1","shm_klient2","shm_klient3","shm_klient4"};

    int fd=shm_open(shm_txt[nr_klienta-1],O_CREAT | O_RDWR,0777);
    if(fd== -1){
        return printf("\nshm_open failed"),1;
    }
    ftruncate(fd,sizeof (struct klient));
    struct klient*g=mmap(NULL, sizeof(struct klient),PROT_READ | PROT_WRITE,MAP_SHARED,fd,0);
    if(g==MAP_FAILED){
        return printf("\nmmap failed"),1;
    }
    sem_t* sem=sem_open("co",0);
    refresh();
    g->nr_klienta=nr_klienta-1;
    g->pid=getpid();
    g->connect=1;
    g->type=0;
    while(1)
    {
        sem_wait(sem);
        paint_map(g);
        int key =wgetch(stdscr);
        if(key=='q' || key=='Q')break;
        g->key= bot_move(g);
        sem_post(&g->cs);

        if(g->connect==0){
            g->connect=2;
            g->pid=getpid();
            g->nr_klienta=nr_klienta-1;
        }
        usleep(200000);
        refresh();
        wclear(stdscr);
    }
    sem_post(&g->cs);
    g->connect=0;
    g->pid=-1;
    slot[nr_klienta-1]=0;
    munmap(g, sizeof(struct klient));
    munmap(slot, sizeof(int)*PLAYERS);
    close(fd_slot);
    endwin();
}
int bot_move(struct klient*k)
{
    /*
     01234
     ##### 0
     ##### 1
     ##G## 2
     ##### 3
     ##### 4
    */
    //KEY_UP
    int x=2;
    int y=2;
    //ucieczka przed bestia
    if(k->mapa[x+1][y].c=='*' || (k->mapa[x+2][y].c=='*'&&k->mapa[x+1][y].c!='X')){
        if(k->mapa[x][y-1].c!='X')return KEY_UP;
        else if(k->mapa[x][y+1].c!='X')return KEY_DOWN;
        else if(k->mapa[x-1][y].c!='X')return KEY_LEFT;
    }
    if(k->mapa[x-1][y].c=='*' || (k->mapa[x-2][y].c=='*'&&k->mapa[x-1][y].c!='X')){
        if(k->mapa[x][y-1].c!='X')return KEY_UP;
        else if(k->mapa[x][y+1].c!='X')return KEY_DOWN;
        else if(k->mapa[x+1][y].c!='X')return KEY_RIGHT;
    }
    if(k->mapa[x][y+1].c=='*' || (k->mapa[x][y+2].c=='*'&&k->mapa[x][y+1].c!='X')){
        if(k->mapa[x-1][y].c!='X')return KEY_LEFT;
        else if(k->mapa[x+1][y].c!='X')return KEY_RIGHT;
        else if(k->mapa[x][y-1].c!='X')return KEY_UP;
    }
    if(k->mapa[x][y-1].c=='*' || (k->mapa[x][y-2].c=='*'&&k->mapa[x][y-1].c!='X')){
        if(k->mapa[x-1][y].c!='X')return KEY_LEFT;
        else if(k->mapa[x+1][y].c!='X')return KEY_RIGHT;
        else if(k->mapa[x][y+1].c!='X')return KEY_DOWN;
    }
    //pogon za pieniedzmi
    if((k->mapa[x+1][y].c=='c' || k->mapa[x+1][y].c=='t' || k->mapa[x+1][y].c=='T')||k->mapa[x+1][y].c=='D' ||
            ((k->mapa[x+2][y].c=='c' || k->mapa[x+2][y].c=='t' || k->mapa[x+2][y].c=='T'|| k->mapa[x+2][y].c=='D')&&k->mapa[x+1][y].c!='X'))return KEY_RIGHT;
    else if(k->mapa[x][y-1].c=='c'||k->mapa[x][y-1].c=='t'||k->mapa[x][y-1].c=='T'|| k->mapa[x][y-1].c=='D'||
            ((k->mapa[x][y-2].c=='c' || k->mapa[x][y-2].c=='t' || k->mapa[x][y-2].c=='T'|| k->mapa[x][y-2].c=='D')&&k->mapa[x][y-1].c!='X'))return KEY_UP;
    else if(k->mapa[x-1][y].c=='c' || k->mapa[x-1][y].c=='t' || k->mapa[x-1][y].c=='T' || k->mapa[x-1][y].c=='D'||
    ((k->mapa[x-2][y].c=='c' || k->mapa[x-2][y].c=='t' || k->mapa[x-2][y].c=='T'|| k->mapa[x-2][y].c=='D')&&k->mapa[x-1][y].c!='X'))return KEY_LEFT;
    else if(k->mapa[x][y+1].c=='c' || k->mapa[x][y+1].c=='t' || k->mapa[x][y+1].c=='T'|| k->mapa[x][y+1].c=='D'||
    ((k->mapa[x][y+2].c=='c' || k->mapa[x][y+2].c=='t' || k->mapa[x][y+2].c=='T'|| k->mapa[x][y+2].c=='D')&&k->mapa[x][y+1].c!='X'))return KEY_DOWN;


        int los=losuj();
        if(los==KEY_UP && k->mapa[x][y-1].c!='X')return KEY_UP;
        else if(los==KEY_DOWN && k->mapa[x][y+1].c!='X')return KEY_DOWN;
        else if(los==KEY_RIGHT && k->mapa[x+1][y].c!='X')return KEY_RIGHT;
        else if(los==KEY_LEFT && k->mapa[x-1][y].c!='X')return KEY_LEFT;

    return losuj();
}
int losuj()
{
    int t=rand()%4;
    if(t==0)return KEY_UP;
    else if(t==1)return KEY_DOWN;
    else if(t==2)return KEY_LEFT;
    else return KEY_RIGHT;
}
void init_ncurses()
{
    initscr();
    timeout(1);
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
}
void paint_map(struct klient*k)
{
    start_color();
    init_pair(1,COLOR_WHITE,COLOR_MAGENTA);
    init_pair(2,COLOR_WHITE,COLOR_GREEN);
    init_pair(3,COLOR_BLACK,COLOR_YELLOW);
    init_pair(4,COLOR_RED,COLOR_BLACK);
    int n=0;
    for(int i=0;i<5;i++)
    {
        for(int j=0;j<5;j++)
        {
            if(k->mapa[i][j].curr.y>=0 && k->mapa[i][j].curr.x>=0){
                wmove(stdscr, k->mapa[i][j].curr.y, k->mapa[i][j].curr.x);

                if(isdigit(k->mapa[i][j].c))n=1;
                if(k->mapa[i][j].c=='A')n=2;
                if(k->mapa[i][j].c=='c' || k->mapa[i][j].c=='t'
                   || k->mapa[i][j].c=='T' ||k->mapa[i][j].c=='D')n=3;
                if(k->mapa[i][j].c=='*')n=4;

                if(n>0){
                    attron(COLOR_PAIR(n));
                    addch(k->mapa[i][j].c);
                    attroff(COLOR_PAIR(n));
                } else{
                    addch(k->mapa[i][j].c);
                }
                n=0;
            }
        }
    }
    mvprintw(1,55,"Server's PID: %d",k->pid);
    mvprintw(2,55,"Campsite X/Y: unknown");
    mvprintw(3,55,"Round number: %d",k->round);
    mvprintw(5,54,"Player:");
    mvprintw(6,55,"Number:   %d",k->nr_klienta+1);
    mvprintw(7,55,"Type:     CPU",2);
    mvprintw(8,55,"Curr X/Y: %02d/%02d",k->g.curr.x,k->g.curr.y);
    mvprintw(9,55,"Deaths: %d",k->g.deaths);
    mvprintw(11,55,"Coins carried: %d",k->g.coins_found);
    mvprintw(12,55,"Coins brought: %d",k->g.coins_brought);
    mvprintw(14,55,"Legend:");
    mvprintw(15,55,"1234 - players");
    mvprintw(16,55,"X    - wall");
    mvprintw(17,55,"#    - bushes (slow down)");
    mvprintw(18,55,"*    - wild beast");
    mvprintw(19,55,"c    - one coin");
    mvprintw(20,55,"t    - treasure (10 coins)");
    mvprintw(21,55,"T    - large treasure (50 coins)");
    mvprintw(22,55,"D    - dropped treasure");
    mvprintw(23,55,"A    - campsite");
}

