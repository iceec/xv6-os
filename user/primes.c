#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
int  is_prime(int x);
int main()
{

    int b[2];
    pipe(b);
    for(int i=2;i<35;++i)
    {
        if(is_prime(i))
        {
            write(b[1],&i,sizeof(i));
        }
        
    }
   
    while(1)
    {

         int num=fork();
        if(num==0)
        {   
            int my_print;
             close(b[1]);   //这个地方一定要先关闭 不然 不行
            if(read(b[0],&my_print,sizeof(int)))
            {
                printf("prime %d\n",my_print);
            }
            else
            {

                exit(0);
            }
            int c[2];
            pipe(c);
           
            while(read(b[0],&my_print,sizeof(int)))
            {   
                write(c[1],&my_print,sizeof(int));
            } 
            close(b[0]);
            b[0]=c[0];
            b[1]=c[1];
        }
        else
        {
            close(b[0]);
            close(b[1]);
            wait((int *)0);
            exit(0);
        }
    }
    
}

int is_prime(int x)
{
    for(int i=2;i*i<=x;++i)
    {
        if(x%i==0)
            return 0;
    }
    return 1;
}