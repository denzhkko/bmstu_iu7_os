#include <stdio.h>   // printf, fprintf, sleep
#include <stdlib.h>  // exit
#include <unistd.h>  // fork

#define ERR_OK 0
#define ERR_FORK 1

#define CHILD_CNT 2
#define CHILD_SLP 2

int main() {
  int child_pids[CHILD_CNT] = {0};

  printf("parent born : PID = %d ; PPID = %d ; GROUP = %d\n", getpid(),
         getppid(), getpgrp());

  for (unsigned child_i = 0; child_i < CHILD_CNT; ++child_i) {
    int pid = fork();

    if (-1 == pid) {
      fprintf(stderr, "Can't fork\n");
      exit(ERR_FORK);
    } else if (0 == pid) {
      // child
      printf("child%u born : PID = %d ; PPID = %d ; GROUP = %d\n", child_i,
             getpid(), getppid(), getpgrp());

      sleep(CHILD_SLP);

      printf("child%u died : PID = %d ; PPID = %d ; GROUP = %d\n", child_i,
             getpid(), getppid(), getpgrp());

      exit(ERR_OK);
    } else {
      // parent
      child_pids[child_i] = pid;
      printf("parent mess : PID = %d ; CHILD_PID = %d\n", getpid(), pid);
    }
  }

  printf("parent died : PID = %d ; PPID = %d ; GROUP = %d\n", getpid(),
         getppid(), getpgrp());
}
