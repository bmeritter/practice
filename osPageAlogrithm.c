#include<stdio.h>
#include<stdlib.h>

#define TRUE 1
#define FALSE 0
#define INVALID -1
#define NUL 0
#define total_instruction 320  //指令流长
#define total_vp 32          //虚页长
#define clear_period 50     //清零周期

typedef struct pl_type{ //页面信息
    int pn;     //页号
    int pfn;    // 物理块号
    int counter;    // 一个周期内访问该页面的次数
    int time;       // time为访问时间
}pl_type;

pl_type pl[total_vp];  //页面信息数组

typedef struct pt_struct{      //页表项
    int pn;  //页号
    int pfn;  //物理块号
    struct pt_struct *next;
}pt_type;

pt_type pt[total_vp];//页表（链式存储）

pt_type *freepf_head;//空闲页表项头指针
pt_type  *busypf_head, *busypf_tail;//已分配出去的页表项（忙页表项）的头指针、尾指针

int diseffect;//未命中次数
int a[total_instruction];//指令流数组
int page[total_instruction];//页数组   每条指令所属的页号
int offset[total_instruction];//页内偏移量      每页装入10条指令后取模运算页号偏移

/*
Name:  void  initialize(inttotal_pf)
Achieve:初始化相关数据结构
*/
void initialize(int total_pf)
{
    int i;
    diseffect=0;

    for(i=0;i<total_vp;i++)
    {
        pl[i].pn=i;//置页面信息中的页号
        pl[i].pfn=INVALID;   //该页面所对应的物理块号为-1，表示该页还未调入内存使用
        pl[i].counter=0;//页面信息中的访问次数为0，
        pl[i].time=-1; //页面信息中的访问时间为-1
    }
    for(i=1;i<total_pf;i++)
    {
        pt[i-1 ].next=&pt[i];//建立pt[i-1]和pt[i]之间的连接
        pt[i-1].pfn=i-1;
    }

    pt[total_pf-1].next=NUL;//最后一个页表项的指针域置为-1
    pt[total_pf-1].pfn=total_pf-1;
    freepf_head=&pt[0];    //空闲页表队列的头指针为pt[0]
}

/*
Name:void FIFO(inttotal_pf)
Achieve:先进先出法（Fisrt In First Out）
参数total_pf表示物理内存块数
    如果一个数据最先进入缓存中，则应该最早被淘汰
    每次替换最先进入内存的页面

*/
void FIFO(int total_pf)
{
    int i;
    pt_type *p;
    initialize(total_pf); //初始化页面信息、页表
    busypf_head=busypf_tail=NULL; //忙页表项队列头，队列尾

    for(i=0;i<total_instruction;i++)
    {
        //第i个页面未命中（第i个页面页面不在页表中）
        if(pl[page[i]].pfn==INVALID)
        {
            diseffect++;  //未命中次数

            //页表中无空闲页表项时
            if(freepf_head == NULL)
            {
                p=busypf_head->next;
                pl[busypf_head->pn].pfn=INVALID;//将忙页表项队列的第一个页面从页表中调出
                freepf_head=busypf_head;  //释放忙页面队列的第一个页面
                freepf_head->next=NULL;
                busypf_head=p;
            }


            /*

            */
            p=freepf_head->next;
            freepf_head->pn=page[i];//将要调入页表的页面的页号赋值给空闲页表项首项的页号
            pl[page[i]].pfn=freepf_head->pfn;
            //将空闲页表项中相应页号所对应的物理块号赋值给将要调入页表的页面的物理块号
            freepf_head->next=NULL; //相应空闲页表项已被占用

            if(busypf_tail==NULL)
            {
                busypf_tail=busypf_head=freepf_head;
            }
            else
            {
                busypf_tail->next=freepf_head;  //空闲页面减少一个
                busypf_tail=freepf_head;
            }

            freepf_head=p;
        }
    }

    printf("%6.3f",1-(float)diseffect/320);
}

/*
Name:  void LRU (inttotal_pf)
Achieve: 最近最久未使用（Least Recently Used）
    如果一个数据在最近一段时间没有被访问到，那么在将来被访问的可能性也很小
    淘汰最长时间未被使用的页面

*/
void LRU (int total_pf)
{
    int min,minj,i,j,present_time; //minj为最小值下标
    initialize(total_pf);
    present_time=0;

    for(i=0;i<total_instruction;i++)
    {
        //页面失效
        if(pl[page[i]].pfn==INVALID)
        {
            diseffect++;
            //无空闲页面
            if(freepf_head==NULL)
            {
                min=32767;//设置最大值

                //找出time的最小值
                for(j=0;j<total_vp;j++)
                {
                    if(min>pl[j].time&&pl[j].pfn!=INVALID)
                    {
                        min=pl[j].time;
                        minj=j;
                    }
                }

                freepf_head=&pt[pl[minj].pfn];   //空出一个单元
                pl[minj].pfn=INVALID;
                pl[minj].time=0;
                freepf_head->next=NULL;
            }

            pl[page[i]].pfn=freepf_head->pfn; //有空闲页面,改为有效
            pl[page[i]].time=present_time;
            freepf_head=freepf_head->next; //减少一个空闲页面
        }
        else
        {
            pl[page[i]].time=present_time++;//命中则增加该单元的访问次数
        //  present_time++;
        }
    }

    printf("%6.3f",1-(float)diseffect/320);
}

/*
Name:void OPT(inttotal_pf)
Achieve:最佳置换算法（Optimal）
*/
int OPT(int total_pf)
{
    int i,j, max,maxpage,d,dist[total_vp];
    pt_type *t;

    initialize(total_pf);
    for(i=0;i<total_instruction;i++)
    {
        if(pl[page[i]].pfn==INVALID)
        {
            diseffect++;

            if(freepf_head==NULL)
            {
                for(j=0;j<total_vp;j++)
                    if(pl[j].pfn!=INVALID)
                        dist[j]=32767;
                    else
                        dist[j]=0;
                d=1;

                for(j=i+1;j<total_instruction;j++)
                {
                    if(pl[page[j]].pfn!=INVALID)
                        dist[page[j]]=d;
                    d++;
                }

                max=-1;
                for(j=0;j<total_vp;j++)
                    if(max<dist[j])
                    {
                        max=dist[j];
                        maxpage=j;
                    }
                freepf_head=&pt[pl[maxpage].pfn];
                freepf_head->next=NULL;
                pl[maxpage].pfn=INVALID;
            }

            pl[page[i]].pfn=freepf_head->pfn;
            freepf_head=freepf_head->next;
        }
    }
    printf("%6.3f",1-(float)diseffect/320);
    return 0;
}

/*
Name: void LFU(inttotal_pf)
Achieve:最不经常使用法（Least Frequently Used）
    如果一个数据在最近一段时间内使用次数很少，那么在将来一段时间内被使用的可能性也很小
    淘汰一定时期内被访问次数最少的页面

*/
void LFU(int total_pf)
{
    int i,j,min,minpage;  //minpage:记录访问次数最少的页面的下标值

    initialize(total_pf);

    for(i=0;i<total_instruction;i++)
    {
        //页面失效
        if(pl[page[i]].pfn==INVALID)
        {
            diseffect++;

            //无空闲页面
            if(freepf_head==NULL)
            {
                min=32767;
                //获取counter的使用用频率最小的内存
                for(j=0;j<total_vp;j++)
                {
                    if(min>pl[j].counter && pl[j].pfn!=INVALID)
                    {
                        min=pl[j].counter;
                        minpage=j;
                    }
                }

                //释放最不经常访问的页面
                freepf_head=&pt[pl[minpage].pfn];
                pl[minpage].pfn=INVALID;
                pl[minpage].counter=0;
                freepf_head->next=NULL;
            }

            pl[page[i]].pfn=freepf_head->pfn;//有空闲页面,改为有效
            pl[page[i]].counter++;
            freepf_head=freepf_head->next;//减少一个空闲页面
        }
        else
        {
            pl[page[i]].counter++;

        }
    }

    printf("%6.3f",1-(float)diseffect/320);
}

/*
Name: void NUR(inttotal_pf)
Achieve:最近最不经常使用法

    页面被访问后count置1，并定期对count进行置0操作，一段时间后，若count为0,则将其置出

*/
int NUR(int total_pf)
{
    int i,j,dp,cont_flag,old_dp;

    initialize(total_pf);
    dp=0;

    for(i=0;i<total_instruction;i++)
    {
        //页面失效
        if(pl[page[i]].pfn==INVALID)
        {
            diseffect++;

            //无空闲页面
            if(freepf_head==NULL)
            {
                cont_flag = TRUE;
                old_dp = dp;

                while(cont_flag)
                    if(pl[dp].counter==0 && pl[dp].pfn!=INVALID)
                        cont_flag=FALSE;

                    else
                    {
                        dp++;
                        if(dp==total_vp)
                            dp=0;
                        if(dp==old_dp)
                        {
                            for(j=0;j<total_vp;j++)
                                pl[j].counter=0;
                        }
                    }
                    freepf_head=&pt[pl[dp].pfn];
                    pl[dp].pfn=INVALID;
                    freepf_head->next=NULL;
                }
            pl[page[i]].pfn=freepf_head->pfn;
            freepf_head=freepf_head->next;
        }
        else
            pl[page[i]].counter=1;

        //定期清零
        if(i % clear_period==0)
            for(j=0;j<total_vp;j++)
                pl[j].counter=0;
    }

    printf("%6.3f\t",1-(float)diseffect/320);
    return 0;
}

void main()
{
    int s,i;

    srand((int)getpid());//设置随机数种子

    /*
        60% 的指令顺序执行
        20% 的指令的指令均匀分布在前地址部分
        20% 的指令的指令均匀分布在后地址部分
    */
    //产生指令队列
    for(i=0;i<total_instruction;i+=5)
    {
        s=(int)rand()%320;//产生一个0~319的随机数
        a[i]=s;   //任选一指令访问点
        a[i+1]=a[i]+1; //顺序执行一条指令
        a[i+2]=(int)rand()%a[i+1]; //执行前地址指令m'
        a[i+3]=a[i+2]+1;//顺序执行一条指令
        a[i+4]=(int)rand()%(319-a[i+2]-1)+a[i+2]+2;//执行后地址指令
        //a[i+4]=(int)rand()%(319-a[i+3])+a[i+3]+1;
    }

    //将指令序列变换成页地址流
    for(i=0;i<total_instruction;i++)
    {
        page[i]=a[i]/10;//页号
        offset[i]=a[i]%10;//页内偏移量
    }

    printf("Frame\tFIFO\tLRU\tOPT\tLFU\tNUR\n");

    //用户内存工作区从4个页面到32个页面
    for(i=4;i<=32;i++)
    {
        printf("%d\t",i);
        FIFO(i);
        printf("\t");
        LRU(i);
        printf("\t");
        OPT(i);
        printf("\t");
        LFU(i);
        printf("\t");
        NUR(i);
        printf("\n");
    }
}
