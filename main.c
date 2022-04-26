#include <stdio.h>
#include <unistd.h>
#include "server.h"
#include <ncurses.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>
#include <signal.h>
#include <string.h>
int main() {//kompilacja gcc -Wall -g -pedantic -o game main.c server.h server.c -lncurses -pthread -lrt

    init_ncurses();
    struct serwer s;
    int t=serwer_init(&s);
    if(!t){
        printf("nie pobrano mapy");
        return 1;
    }
    //pamiec na sloty
    int fd_slot=shm_open("my_slot",O_CREAT | O_RDWR,0777);
    if(fd_slot== -1){
        endwin();
        return printf("\nshm_open failed"),1;
    }
    ftruncate(fd_slot,sizeof(int)*PLAYERS);
    int*sloty=mmap(NULL, sizeof(int)*PLAYERS,PROT_READ | PROT_WRITE,MAP_SHARED,fd_slot,0);
    if(sloty==MAP_FAILED){
        endwin();
        return printf("\nmmap failed"),1;
    }
    for(int i=0;i<PLAYERS;i++)
        sloty[i]=0;

    char shm_txt[4][20]={"shm_klient1","shm_klient2","shm_klient3","shm_klient4"};
    int fd_p[PLAYERS];
    for(int i=0;i<PLAYERS;i++){
        fd_p[i]=shm_open(shm_txt[i],O_CREAT | O_RDWR,0777);
        if(fd_p[i]== -1){
            endwin();
            return printf("\nshm_open failed"),1;
        }
        ftruncate(fd_p[i],sizeof (struct klient));
        struct klient*ptr=mmap(NULL, sizeof(struct klient),PROT_READ | PROT_WRITE,MAP_SHARED,fd_p[i],0);
        if(ptr==MAP_FAILED){
            endwin();
            return printf("\nmmap failed"),1;
        }
        sem_init(&ptr->cs,1,0);
        sem_init(&ptr->cx,1,0);
        ptr->connect=0;
        ptr->nr_klienta=-1;
        s.klienci[i]=ptr;
    }

    //pamiec na bestie
    int fd_beast= shm_open("shm_beast",O_CREAT | O_RDWR,0777);
    if(fd_beast== -1){
        endwin();
        return printf("\nshm_open failed"),1;
    }
    ftruncate(fd_beast,sizeof(struct monsters));
    struct monsters*mon=mmap(NULL, sizeof(struct monsters),PROT_READ | PROT_WRITE,MAP_SHARED,fd_beast,0);
    if(mon==MAP_FAILED){
        endwin();
        return printf("\nmmap failed"),1;
    }
    sem_init(&mon->ms,1,0);
    sem_init(&mon->mx,1,0);
    sem=sem_open("co",O_CREAT,0777,0);

    paint_all(&s);
    refresh();

    pthread_t klient[PLAYERS];
    for(int i=0;i<PLAYERS;i++){
        pthread_create(&klient[i],NULL,player_pthread,s.klienci[i]);
    }

    while(1)
    {
        int key =wgetch(stdscr);
        if(key=='q' || key=='Q')break;
        else if(key =='c' || key=='t' || key=='T')add_gem(&s,(char)key);
        else if((key=='b' || key=='B') && mon->ile<MON && mon->connect==1){
            add_beast(&s);
            mon->ile+=1;
        }
        for(int i=0;i<PLAYERS;i++){
            get_map(&s,s.klienci[i]);
        }
        if(mon->connect==1){
            get_all_map(mon);//mapa dla bestii
            sem_post(&mon->ms);
            struct timespec interval;
            clock_gettime(CLOCK_REALTIME,&interval);
            interval.tv_sec+=1;
            if(sem_timedwait(&mon->mx,&interval)!=0)mon->connect=0;
            move_beast(&s,mon);
        }
        else{
            s.mon_count=0;
            mon->ile=0;
        }

        ile_graczy(&s,sloty);
        for(int i=0;i<PLAYERS;i++){
            if(s.klienci[i]->connect>0)sem_post(&s.klienci[i]->cs);
        }
        for(int i=0;i<PLAYERS;i++){
            if(s.klienci[i]->connect>0){
                sem_wait(&s.klienci[i]->cx);
                if(s.klienci[i]->connect==1){
                    put_player(&s.gracze[i]);
                    s.klienci[i]->connect=2;
              }
            }
        }
        player_valid(&s);
        for(int i=0;i<PLAYERS;i++){
            player_move(&s,s.klienci[i]->key,i);
        }
        for(int i=0;i<PLAYERS;i++)
        {
            if(s.klienci[i]->connect>0){
                memcpy(&s.klienci[i]->g,&s.gracze[i], sizeof(struct gracz));
            }
        }
        //bestia

        player_valid(&s);
        paint_all(&s);
        usleep(200000);
        s.round_nr++;
        refresh();
    }
    //pamiec wspoldzielona

    sem_unlink("co");

    close(fd_slot);
    close(fd_beast);
    munmap(mon,sizeof (struct monsters));
    munmap(sloty,sizeof(int)*PLAYERS);
    shm_unlink("my_slot");
    shm_unlink("shm_beast");
    for(int i=0;i<PLAYERS;i++)
    {
        close(fd_p[i]);
        sem_close(&s.klienci[i]->cs);
        munmap(s.klienci[i], sizeof(struct klient));
        shm_unlink(shm_txt[i]);
    }
    getch();
    endwin();
    return 0;
}
void init_ncurses()
{
    initscr();
    timeout(1);
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
}
void ile_graczy(struct serwer *s,int *slot){
    s->player_count=0;
    for(int i=0;i<PLAYERS;i++)
    {
        if(s->klienci[i]->connect==0 && s->klienci[i]->pid==-2 && s->klienci[i]->nr_klienta!=-1){
            slot[s->klienci[i]->nr_klienta]=0;
            s->klienci[i]->nr_klienta=-1;
        }
        if(s->klienci[i]->connect>0)s->player_count++;
    }
}
