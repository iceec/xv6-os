#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char*
fmtname(char *path)
{
  static char buf[DIRSIZ+1];
  char *p;

  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  // Return blank-padded name.
  if(strlen(p) >= DIRSIZ)
    return p;
  memmove(buf, p, strlen(p));
  memset(buf+strlen(p), 0, DIRSIZ-strlen(p));
  return buf;
}


void
r_find(char *path,char *name)
{
   char buf[256], *p;
  int fd;
  struct dirent de;
  struct stat st;

  if((fd = open(path, 0)) < 0){
    fprintf(2, "ls: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0){
    fprintf(2, "ls: cannot stat %s\n", path);
    close(fd);
    return;
  }

  switch(st.type){
  case T_FILE:
    //printf("%s   %s %d\n",path,fmtname(path),strlen(fmtname(path)));
    if(strcmp(fmtname(path),name)==0)
    {
        printf("%s\n",path);
    }
    break;
  case T_DIR:
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
      printf("ls: path too long\n");
      break;
    }
    strcpy(buf, path);
    p = buf+strlen(buf);
    *p++ = '/';
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0)
        continue;
    if(strcmp(de.name,".")==0)
            continue;
    if(strcmp(de.name,"..")==0)
            continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      if(stat(buf, &st) < 0){
        printf("ls: cannot stat %s\n", buf);
        continue;
      }
      // printf("%s\n",buf);
      r_find(buf,name);
    }
    break;
  }
  close(fd);
}



int
main(int argc, char *argv[])
{
    if(argc<3)
        exit(0);
    char bf[200];
    strcpy(bf,argv[1]);
    r_find(bf,argv[2]);
    exit(0);

}
