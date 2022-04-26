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
#define MON 5
#define M 25
#define N 52

struct punkt{
    int x;
    int y;
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

void init_ncurses();
void *monster_pthread(void*ptr);
void paint_map(struct monsters*k);
int main() {//gcc -Wall -pedantic -o monster monster.c -lncurses -pthread -lrt

    init_ncurses();
    int fd_beast=shm_open("shm_beast",O_RDWR,0777);
    if(fd_beast== -1){
        endwin();
        return printf("\nshm_open failed"),1;
    }
    ftruncate(fd_beast,sizeof (struct monsters));
    struct monsters*mon=mmap(NULL, sizeof(struct monsters),PROT_READ | PROT_WRITE,MAP_SHARED,fd_beast,0);
    if(mon==MAP_FAILED){
        endwin();
        return printf("\nmmap failed"),1;
    }
    for(int i=0;i<MON;i++)
    {
        sem_init(&mon->mon[i].bs,0,0);
        sem_init(&mon->mon[i].bx,0,0);
    }
    pthread_t bestie[MON];
    for(int i=0;i<MON;i++)
         pthread_create(&bestie[i],NULL,monster_pthread,&mon->mon[i]);

    mon->connect=1;
    while(1)
    {
        sem_wait(&mon->ms);
        paint_map(mon);
        for(int i=0;i<mon->ile;i++)
        {
            sem_post(&mon->mon[i].bs);
        }
        for(int i=0;i<mon->ile;i++)
        {
            sem_wait(&mon->mon[i].bx);
        }
        sem_post(&mon->mx);
        int key =wgetch(stdscr);
        if(key=='q' || key=='Q')break;
        usleep(200000);
        refresh();
        wclear(stdscr);
    }
    mon->connect=0;
    munmap(mon, sizeof(struct monsters));
    endwin();
}

void init_ncurses()
{
    initscr();
    timeout(1);
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
}
void paint_map(struct monsters*k)
{
    for(int i=0;i<M;i++)
    {
        for(int j=0;j<N;j++)
        {
                wmove(stdscr, i, j);
                addch(k->map[i][j]);
        }
    }
}
void *monster_pthread(void*ptr)
{
    struct bestia *mon=(struct bestia*)ptr;
    srand(time(NULL));
    while(true)
    {
        sem_wait(&mon->bs);
        int x=mon->curr.x;
        int y=mon->curr.y;
        //czy gracz na dole
        int flag=0;
        for(int i=0;i<N && mvinch(y+i,x)!='X';i++){
            if(mvinch(y+i,x)=='1' || mvinch(y+i,x)=='2' ||mvinch(y+i,x)=='3' || mvinch(y+i,x)=='4'){
                flag=1;
                break;
            }
        }
        //czy gracz na gorze
        if(!flag){
            for(int i=0;i<N && mvinch(y-i,x)!='X';i++){
                if(mvinch(y-i,x)=='1' || mvinch(y-i,x)=='2' || mvinch(y-i,x)=='3' || mvinch(y-i,x)=='4'){
                    flag=2;
                    break;
                }
            }
        }
        //czy gracz po prawo
        if(!flag){
            for(int i=0;i<N && mvinch(y,x+i)!='X';i++){
                if(mvinch(y,x+i)=='1' || mvinch(y,x+i)=='2' || mvinch(y,x+i)=='3' || mvinch(y,x+i)=='4'){
                    flag=3;
                    break;
                }
            }
        }
        //czy gracz po lewej
        if(!flag){
            for(int i=0;i<N && mvinch(y,x-i)!='X';i++){
                if(mvinch(y,x-i)=='1' || mvinch(y,x-i)=='2' || mvinch(y,x-i)=='3' || mvinch(y,x-i)=='4' ){
                    flag=4;
                    break;
                }
            }
        }
        if(flag==0)
        {
            while(1)
            {
                int t=rand()%4;
                if(t==0 && mvinch(mon->curr.y,mon->curr.x+1)!='X'){
                    mon->key=KEY_RIGHT;
                    break;
                }
                else if(t==1 && mvinch(mon->curr.y,mon->curr.x-1)!='X'){
                    mon->key=KEY_LEFT;
                    break;
                }
                else if(t==2 && mvinch(mon->curr.y+1,mon->curr.x)!='X'){
                    mon->key=KEY_DOWN;
                    break;
                }
                else if(t==3 && mvinch(mon->curr.y-1,mon->curr.x)!='X'){
                    mon->key=KEY_UP;
                    break;
                }
            }
        }
        else{
            if(flag==1)mon->key=KEY_DOWN;
            else if(flag==2)mon->key=KEY_UP;
            else if(flag==3)mon->key=KEY_RIGHT;
            else if(flag==4)mon->key=KEY_LEFT;
        }
        sem_post(&mon->bx);
    }
}

