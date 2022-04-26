#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
int serwer_init(struct serwer* p)
{
    FILE*fp;
    fp=fopen("maze.txt","r");
    if(!fp)return 0;
    p->campsite.x=23;
    p->campsite.y=11;
    p->pid=getpid();
    p->round_nr=0;
    p->mon_count=0;
    p->player_count=0;
    int c;
    for(int i=0,j;i<M;i++)
    {
        for(j=0;j<N;j++)
        {
            c=fgetc(fp);
            if(c==EOF)break;
            p->map[i][j]=(char)c;
        }
        if(c==EOF)break;
    }
    fclose(fp);
    paint_maze(p);
    //skarby
    srand(time(NULL));
    for(int i=0;i<GEM+GEM;i++)
    {
        p->skarby[i].c=0;
        p->drop[i].curr.x=0;
        p->drop[i].curr.y=0;
        p->drop[i].carried=0;
        p->drop[i].c=0;
        if(i<GEM)
        {
            while(1)
            {
                p->skarby[i].curr.x=rand()%(N-3)+2;
                p->skarby[i].curr.y=rand()%(M-3)+2;
                if(mvinch(p->skarby[i].curr.y, p->skarby[i].curr.x)==' ' && p->skarby[i].curr.x>0 && p->skarby[i].curr.y>0)break;
            }
            int skarb=rand()%3;
            if(skarb==0)p->skarby[i].c='c';
            else if(skarb==1)p->skarby[i].c='t';
            else p->skarby[i].c='T';
        }
    }
    return 1;
}
void paint(struct serwer*p)
{
    start_color();
    init_pair(2,COLOR_WHITE,COLOR_GREEN);
    init_pair(3,COLOR_BLACK,COLOR_YELLOW);
    init_pair(4,COLOR_RED,COLOR_BLACK);

    paint_maze(p);
    //campsite
    wmove(stdscr, p->campsite.y, p->campsite.x);
    attron(COLOR_PAIR(2));
    addch('A');
    attroff(COLOR_PAIR(2));
    //skarby
    for(int i=0;i<GEM+GEM;i++)
    {
        if(p->skarby[i].c!=0)
        {
            wmove(stdscr, p->skarby[i].curr.y, p->skarby[i].curr.x);
            attron(COLOR_PAIR(3));
            addch(p->skarby[i].c);
            attroff(COLOR_PAIR(3));
        }
    }
    //porzucony skarb
    for(int i=0;i<GEM;i++)
    {
        if(p->drop[i].c=='D')
        {
            wmove(stdscr, p->drop[i].curr.y, p->drop[i].curr.x);
            attron(COLOR_PAIR(3));
            addch(p->drop[i].c);
            attroff(COLOR_PAIR(3));
        }
    }
    //bestia
    for(int i=0;i<p->mon_count;i++)
    {
        wmove(stdscr, p->monster[i].curr.y, p->monster[i].curr.x);
        attron(COLOR_PAIR(4));
        addch('*');
        attroff(COLOR_PAIR(4));
    }
}
void paint_maze(struct serwer*p)
{
    for(int i=0;i<M;i++)
    {
        for(int j=0;j<N;j++)
        {
            wmove(stdscr, i, j);
            addch(p->map[i][j]);
        }
    }
}
void paint_legend()
{
    mvprintw(15,55,"Legend:");
    mvprintw(16,55,"1234 - players");
    mvprintw(17,55,"X    - wall");
    mvprintw(18,55,"#    - bushes (slow down)");
    mvprintw(19,55,"*    - wild beast");
    mvprintw(20,55,"c    - one coin");
    mvprintw(21,55,"t    - treasure (10 coins)");
    mvprintw(22,55,"T    - large treasure (50 coins)");
    mvprintw(23,55,"D    - dropped treasure");
    mvprintw(24,55,"A    - campsite");

}
void paint_player(struct serwer*s,int n)
{
    start_color();
    init_pair(1,COLOR_WHITE,COLOR_MAGENTA);
    wmove(stdscr, s->klienci[n]->g.curr.y, s->klienci[n]->g.curr.x);
    attron(COLOR_PAIR(1));
    addch((n+1)+'0');
    attroff(COLOR_PAIR(1));
}
void paint_all(struct serwer*p)
{
    paint(p);
    //paint_maze(p);
    for(int i=0;i<PLAYERS;i++){
        if(p->klienci[i]->connect==2)paint_player(p,i);
    }
    //info z boku
    mvprintw(1,55,"Server's PID: %d",p->pid);
    mvprintw(1,80,"ILE: %d",p->player_count);
    mvprintw(2,55,"Campsite X/Y: %2d/%2d",p->campsite.x,p->campsite.y);
    mvprintw(3,55,"Round number: %d",p->round_nr);
    mvprintw(5,55,"Parameter:");
    mvprintw(6,55,"PID");
    mvprintw(7,55,"Type");
    mvprintw(8,55,"Curr X/Y");
    mvprintw(9,55,"Deaths");
    mvprintw(11,55,"Coins");
    mvprintw(12,57,"carried");
    mvprintw(13,57,"brought");
    for(int i=0;i<PLAYERS;i++)
    {
        if(p->klienci[i]->connect!=0){
            mvprintw(5,68+i*10,"Player %d",i+1);
            mvprintw(6,68+i*10,"%d",p->klienci[i]->pid);
            if(p->klienci[i]->type==1)mvprintw(7,68+i*10,"HUMAN");
            else if(p->klienci[i]->type==0)mvprintw(7,68+i*10,"CPU");
            mvprintw(8,68+i*10,"%02d/%02d",p->gracze[i].curr.x,p->gracze[i].curr.y);
            mvprintw(9,68+i*10,"%d",p->gracze[i].deaths);
            mvprintw(12,68+i*10,"%d",p->gracze[i].coins_found);
            mvprintw(13,68+i*10,"%d",p->gracze[i].coins_brought);
        }
        else{
            mvprintw(5,68+i*10,"Player %d",i+1);
            mvprintw(6,68+i*10,"-");
            mvprintw(7,68+i*10,"-");
            mvprintw(8,68+i*10,"--/--");
            mvprintw(9,68+i*10,"-");
            mvprintw(12,68+i*10,"-");
            mvprintw(13,68+i*10,"-");
        }

    }
    paint_legend();
}

int player_move(struct serwer*s,int key,int nr)
{
    if(s->gracze[nr].krzak==1){
        s->gracze[nr].krzak=0;
        return 0;
    }

    switch (key) {
        case KEY_UP:
            if(mvinch(s->gracze[nr].curr.y-1, s->gracze[nr].curr.x)!='X')
            {
                if(mvinch(s->gracze[nr].curr.y-1, s->gracze[nr].curr.x)=='#') s->gracze[nr].krzak++;
                s->gracze[nr].curr.y--;
            }
            return 1;
        case KEY_DOWN:
            if(mvinch(s->gracze[nr].curr.y+1, s->gracze[nr].curr.x)!='X')
            {
                if(mvinch(s->gracze[nr].curr.y+1, s->gracze[nr].curr.x)=='#') s->gracze[nr].krzak++;
                s->gracze[nr].curr.y++;
            }
            return 1;
        case KEY_RIGHT:
            if(mvinch(s->gracze[nr].curr.y, s->gracze[nr].curr.x+1)!='X')
            {
                if(mvinch(s->gracze[nr].curr.y, s->gracze[nr].curr.x+1)=='#') s->gracze[nr].krzak++;
                s->gracze[nr].curr.x++;
            }
            return 1;
        case KEY_LEFT:
            if(mvinch(s->gracze[nr].curr.y, s->gracze[nr].curr.x-1)!='X')
            {
                if(mvinch(s->gracze[nr].curr.y, s->gracze[nr].curr.x-1)=='#') s->gracze[nr].krzak++;
                s->gracze[nr].curr.x--;
            }
            return 1;
        default:
            return 0;
    }
}

void player_valid(struct serwer*s)//do zmiany
{
    for(int n=0;n<s->player_count;n++)
    {
        for(int i=0;i<GEM+GEM;i++)
        {
            //skarby
            if(s->skarby[i].curr.y==s->gracze[n].curr.y && s->skarby[i].curr.x==s->gracze[n].curr.x && s->skarby[i].c!=0){
                if(s->skarby[i].c=='c')s->gracze[n].coins_found+=1;
                else if(s->skarby[i].c=='t')s->gracze[n].coins_found+=10;
                else s->gracze[n].coins_found+=50;
                if(i<GEM)
                {
                    while(1)
                    {
                        s->skarby[i].curr.x=rand()%(N-2)+1;
                        s->skarby[i].curr.y=rand()%(M-2)+1;
                        if(mvinch(s->skarby[i].curr.y, s->skarby[i].curr.x)==' ')break;
                    }
                    int skarb=rand()%3;
                    if(skarb==0)s->skarby[i].c='c';
                    else if(skarb==1)s->skarby[i].c='t';
                    else s->skarby[i].c='T';
                }
                else{
                    s->skarby[i].curr.x=0;
                    s->skarby[i].curr.y=0;
                    s->skarby[i].c=0;
                }
                break;
            }
        }
        //porzucony skarb zbieranie
        for(int i=0;i<GEM;i++)
        {
            if(s->drop[i].curr.x==s->gracze[n].curr.x && s->drop[i].curr.y==s->gracze[n].curr.y ){
                s->gracze[n].coins_found+=s->drop[i].carried;
                s->drop[i].carried=0;
                s->drop[i].curr.x=0;
                s->drop[i].curr.y=0;
                s->drop[i].c=0;
                break;
            }
        }
        //campsite
        if(s->campsite.y==s->gracze[n].curr.y && s->campsite.x==s->gracze[n].curr.x){
            s->gracze[n].coins_brought+=s->gracze[n].coins_found;
            s->gracze[n].coins_found=0;
        }
        for(int j=0;j<s->mon_count;j++)
        {
            //bestia
            if(s->gracze[n].curr.x==s->monster[j].curr.x && s->gracze[n].curr.y==s->monster[j].curr.y){
                s->gracze[n].curr.x=s->gracze[n].start.x;
                s->gracze[n].curr.y=s->gracze[n].start.y;
                s->gracze[n].deaths++;
                //porzucony skarb
                for(int i=0;i<GEM;i++)
                {
                    if(s->drop[i].c==0 && s->gracze[n].coins_found>0){
                        s->drop[i].curr.x=s->monster[j].curr.x;
                        s->drop[i].curr.y=s->monster[j].curr.y;
                        s->drop[i].carried=s->gracze[n].coins_found;
                        s->drop[i].c='D';
                        s->gracze[n].coins_found=0;
                        break;
                    }
                }
            }
        }
    }

    for(int i=0;i<PLAYERS-1;i++)
    {
        for(int j=i+1;j<PLAYERS;j++)
        {
            if(s->gracze[i].curr.x==s->gracze[j].curr.x && s->gracze[i].curr.y==s->gracze[j].curr.y && s->klienci[i]->connect>0 && s->klienci[j]->connect>0){
                for(int k=0;k<GEM;k++)
                {
                    if(s->drop[k].c==0 && s->gracze[i].coins_found+s->gracze[j].coins_found>0){
                        s->drop[k].curr.x=s->gracze[i].curr.x;
                        s->drop[k].curr.y=s->gracze[i].curr.y;
                        s->drop[k].carried=s->gracze[i].coins_found+s->gracze[j].coins_found;
                        s->drop[k].c='D';
                        s->gracze[i].coins_found=0;
                        s->gracze[j].coins_found=0;
                        break;
                    }
                }
                s->gracze[i].curr.x=s->gracze[i].start.x;
                s->gracze[i].curr.y=s->gracze[i].start.y;
                s->gracze[i].deaths++;
                s->gracze[j].curr.x=s->gracze[j].start.x;
                s->gracze[j].curr.y=s->gracze[j].start.y;
                s->gracze[j].deaths++;
            }
        }
    }
}

int put_player(struct gracz*g)
{
    srand(time(NULL));
    if(!g)return 0;
    g->coins_brought=0;
    g->coins_found=0;
    g->deaths=0;
    g->krzak=0;
    //losowanie miejsca start
    while(1)
    {
        g->start.x=rand()%(N-2)+1;
        g->start.y=rand()%(M-2)+1;
        if(mvinch(g->start.y, g->start.x)==' ')break;
    }
    g->curr.x=g->start.x;
    g->curr.y=g->start.y;
    return 1;
}

void add_gem(struct serwer*s,char a)
{
    for(int i=GEM;i<GEM+GEM;i++)
    {
        if(s->skarby[i].c==0){
            while(1)
            {
                s->skarby[i].curr.x=rand()%(N-3)+2;
                s->skarby[i].curr.y=rand()%(M-3)+2;
                if(mvinch(s->skarby[i].curr.y, s->skarby[i].curr.x)==' ')break;
            }
            s->skarby[i].c=a;
            break;
        }
    }
}


void *player_pthread(void*ptr)
{
    struct klient *s=(struct klient*)ptr;
    while(1)
    {
        sem_wait(&s->cs);
        sem_post(sem);
        struct timespec interval;
        clock_gettime(CLOCK_REALTIME,&interval);
        interval.tv_sec+=1;
        if(sem_timedwait(&s->cs,&interval)!=0)
        {
            s->connect=0;
            s->pid=-2;
        }
        sem_post(&s->cx);
    }
}

void get_map(struct serwer*s,struct klient *k)
{
    int x=k->g.curr.x-2;
    int y=k->g.curr.y-2;
    for(int i=0;i<5;i++,x++){
        for(int j=0;j<5;j++,y++){
            k->mapa[i][j].curr.y=y;
            k->mapa[i][j].curr.x=x;
            k->mapa[i][j].c=mvinch(y,x);
        }
        y=k->g.curr.y-2;
    }
    k->round=s->round_nr;
}
void get_all_map(struct monsters *mon)
{
    for(int i=0;i<M;i++)
    {
        for(int j=0;j<N;j++)
        {
            mon->map[i][j]=mvinch(i,j);
        }
    }
}
void add_beast(struct serwer*s)
{
    if(s->mon_count<MON){
        while(1)
        {
            s->monster[s->mon_count].curr.x=rand()%(N-3)+1;
            s->monster[s->mon_count].curr.y=rand()%(M-3)+1;
            if(mvinch(s->monster[s->mon_count].curr.y,  s->monster[s->mon_count].curr.x)==' ')break;
        }
        s->mon_count++;
    }
}
void move_beast(struct serwer*s,struct monsters*mon)
{
    for(int i=0;i<mon->ile;i++)
    {
        if(mon->mon[i].key==KEY_UP && mvinch(s->monster[i].curr.y-1,s->monster[i].curr.x)!='X'){
            s->monster[i].curr.y--;
        }
        else if(mon->mon[i].key==KEY_DOWN && mvinch(s->monster[i].curr.y+1,s->monster[i].curr.x)!='X'){
            s->monster[i].curr.y++;
        }
        else if(mon->mon[i].key==KEY_LEFT && mvinch(s->monster[i].curr.y,s->monster[i].curr.x-1)!='X'){
            s->monster[i].curr.x--;
        }
        else if(mon->mon[i].key==KEY_RIGHT && mvinch(s->monster[i].curr.y,s->monster[i].curr.x+1)!='X'){
            s->monster[i].curr.x++;
        }
        mon->mon[i].curr.x=s->monster[i].curr.x;
        mon->mon[i].curr.y=s->monster[i].curr.y;
    }
}