#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main() {
  int b[2];
  char buf[10];
  pipe(b);
  int number = fork();

  if (number == 0) {
    // child
    char receive;
    if (read(b[0], buf, 1)) {
      printf("%d: received ping\n", getpid());
    }
    // close(b[0]);
    write(b[1], &receive, 1);
    // close(b[1]);
    exit(0);
  }
  char temp = 'h';
  write(b[1], &temp, 1);
  // close(b[1]);
  wait((int *)0);

  if (read(b[0], buf, 1)) {
    printf("%d: received pong\n", getpid());
  }
  // close(b[0]);

  exit(0);
}