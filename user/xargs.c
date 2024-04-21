#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

int main(int argc,char *argv[])
{
    if(argc<2)
        exit(0);
    if(argc>MAXARG+1)
        exit(0);

   char * info[MAXARG];
   char buf[1024];
   int j=0,i=0;

   for(int m=1;m<argc;++m,++i)
    info[i]=argv[m];

while(1)
{
j=0;
while(read(0,&buf[j],1)>0)
{      
     if(buf[j]=='\n')
        break;
    ++j;
}
if(j==0)
    break;

buf[j]=0;
info[i]=buf;
if(fork()==0)
{   //for(int m=0;m<i;++m)
       // printf("%s %d\n",info[m],strlen(info[m]));
    exec(argv[1],info);
}
    else
        wait((int *)0);
}
exit(0);
}
