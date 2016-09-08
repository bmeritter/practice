#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>
#include <errno.h>

#define MAX_LEN 1024

char *filename[MAX_LEN]; //存储文件名
long inode[MAX_LEN];

void do_lsl(char *);
void do_lsa(char *,char, char);
void dostat(char *);
void show_file_info(char *,struct stat *); //输出文件信息
void mode_to_letters(int,char[]);  //文件属性字段
char * uid_to_name(uid_t);  // 用户名
char * gid_to_name(gid_t);  // 组名
void filename_sort();
void show_filename(char);
void stat_info(char *, struct stat *);

void main(int argc, char *argv[])
{
    char *param;
    char *path;
    char ch1, ch2;
    if(argc <= 1)
    {
        printf("请至少输入一个参数!\n");
        return;
    }
    else if(argc == 2)
    {
        param = argv[1];
        path = ".";
    }
    else
    {
        param = argv[1];
        path = argv[2];
        chdir(path); //转换路径
    }

    ch1 = param[1];
    if(strlen(param)>=3)
    {
        ch2 = param[2];
        printf("--%d\n",strlen(param));
    }
    system("pwd");

    switch(ch1)
    {
        case 'a':
            do_lsa(path,ch1,ch2);
            break;
        case 'l':
            do_lsl(path);
            break;
        case 'i':
            do_lsa(path,ch1,ch2);
            break;
    }

}

void do_lsa(char dirname[],char ch1, char ch2)
{
    //将文件名存入filename中
    struct stat info;
    int i = 0, len;
    DIR *dir_ptr;
    struct dirent *direntp;

    if((dir_ptr = opendir(dirname)) == NULL)
    {
        fprintf(stderr, "ls:cannot open %s\n",dirname);
    }
    else
    {
        while((direntp = readdir(dir_ptr)) != NULL)
        {
            inode[i] = direntp->d_ino;
            len = sizeof(direntp->d_name);
            filename[i] = (char *)malloc(len+3);
            strcpy(filename[i],direntp->d_name);

            if(ch2 == 'F')
            {
                if(stat(filename[i],&info) == -1)
                {
                    printf("%d\n",errno);
                    return;
                }
                else
                {
                    stat_info(filename[i],&info);
                }

            }
            i++;
        }
    }
        show_filename(ch1);
}

//-a 显示文件名
void show_filename(char ch)
{
    int i,j;
    filename_sort();
    for(i=0; filename[i] != NULL; i++)
    {   if(ch == 'i')
            printf("%ld\t",inode[i]);
        printf("%s\n", filename[i]);
    }
}

void stat_info(char *filename, struct stat *info)
{
    int mode = info->st_mode;
    if(S_ISDIR(mode))
    {
        strcat(filename, "/");
        return;
    }

    if(S_ISFIFO(mode))
    {
        strcat(filename, "|");
        return;
    }

    if(S_ISSOCK(mode))
    {
        strcat(filename, "=");
        return;
    }

    if(mode & S_IXUSR)
    {
        strcat(filename, "*");
        return;
    }
}

//排序
void filename_sort()
{
    int i,j;
    char *tmp;
    for(i=0; filename[i]!=NULL; i++)
    {
        for(j=0; filename[j]!=NULL; j++)
        {
            if(strcmp(filename[i],filename[j]) <= 0)
            {
                tmp = (char *)malloc(sizeof(filename[j]+1));
                strcpy(tmp, filename[j]);
                strcpy(filename[j], filename[i]);
                strcpy(filename[i], tmp);
            }
        }
    }
}



void do_lsl(char dirname[])
{
        DIR *dir_ptr;   //路径变量
        struct dirent *direntp;     //存储路径下一 个子项信息的结构体
        if((dir_ptr=opendir(dirname))==NULL)
            fprintf(stderr,"ls:cannot open %s\n",dirname);
        else
    {
            while((direntp=readdir(dir_ptr))!=NULL)
                {
            dostat(direntp->d_name);//以文件名为参数，读取文件属性
        }

        closedir(dir_ptr);
        }
}

//stat读取任意类型文件属性
void dostat(char *filename)
{
        struct stat info;
        if(lstat(filename,&info)==-1)//获取连接文件自身的属性
            perror("lstat");
        else
            show_file_info(filename,&info);
}

//输出文件信息
void show_file_info(char *filename,struct stat *info_p)
{
        char modestr[11];
        mode_to_letters(info_p->st_mode,modestr);//将文件类型及权限由数字转换为字母

        printf("%-12s",modestr); //类型及各种权限
        printf("%-4d",(int)info_p->st_nlink); //硬链接个数
        printf("%-8s",uid_to_name(info_p->st_uid));  //参数为用户ID，输出用户名
        printf("%-8s",gid_to_name(info_p->st_gid)); //参数为组ID，输出组名
        printf("%-8ld",(long)info_p->st_size); //大小
        time_t timelong=info_p->st_mtime;  //最后一次修改时间
        //localtime是 把从1970-1-1零点零分到当前时间系统所偏移的秒数时间转换为本地时间
        struct tm *htime=localtime(&timelong);   //
        printf("%-4d-%02d-%02d %02d:%02d",htime->tm_year+1900,htime->tm_mon+1,htime->tm_mday,htime->tm_hour,htime->tm_min);
        printf(" %s\n",filename); //文件名
}

//转换文件类型及权限
void mode_to_letters(int mode,char str[])
{
        strcpy(str,"----------");
        if(S_ISDIR(mode))
        str[0]='d';//是否为目录
        if(S_ISCHR(mode))
        str[0]='c';//是否为字符设备文件
        if(S_ISBLK(mode))
        str[0]='b';//是否是块设备文件
    //文件拥有者权限
        if(mode & S_IRUSR)
        str[1]='r';//可读
        if(mode & S_IWUSR)
        str[2]='w';//可写
        if(mode & S_IXUSR)
        str[3]='x';//可执行
        //用户组权限
        if(mode & S_IRGRP)
        str[4]='r';
        if(mode & S_IWGRP)
        str[5]='w';
        if(mode & S_IXGRP)
        str[6]='x';
    //其他用户权限
        if(mode & S_IROTH)
        str[7]='r';
        if(mode & S_IWOTH)
        str[8]='w';
        if(mode & S_IXOTH)
        str[9]='x';
}

//通过getpwuid()可通过用户UID查看用户基本信息
char * uid_to_name(uid_t uid)
{   //uid 类型unsigned short
        struct passwd *pw_str;
        static char numstr[10];
        if((pw_str=getpwuid(uid))==NULL)
    {
        //字符串格式化命令，主要功能是把格式化的数据写入某个字符串中。
            sprintf(numstr,"%d",uid); //如果没有获得用户名，则直接把uid当作用户名
            return numstr;
        }
        else
            return pw_str->pw_name;
}

//通过getgrgid()可通过用户组GID查看用户基本信息
char * gid_to_name(gid_t gid)
{
        struct group *grp_ptr;
        static char numstr[10];
        if((grp_ptr=getgrgid(gid))==NULL)
    {
            sprintf(numstr,"%d",gid);
            return numstr;
        }
        else
            return grp_ptr->gr_name;
}
