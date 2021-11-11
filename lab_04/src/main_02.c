#include <stdio.h>     // printf, fprintf, sleep
#include <stdlib.h>    // exit
#include <sys/wait.h>  // wait
#include <unistd.h>    // fork

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

    if (pid == -1) {
      fprintf(stderr, "Can't fork\n");
      exit(ERR_FORK);
    } else if (pid == 0) {
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
    }
  }

  for (unsigned child_i = 0; child_i < CHILD_CNT; ++child_i) {
    int status, stat_val = 0;

    printf("parent waiting\n");
    pid_t childpid = wait(&status);
    printf("parent waited : child process (PID = %d) finished. status: %d\n",
           childpid, status);

    if (WIFEXITED(stat_val)) {
      printf("parent talk : child process #%d finished with code: %d\n",
             child_i + 1, WEXITSTATUS(stat_val));
    } else if (WIFSIGNALED(stat_val)) {
      printf(
          "parent talk : child process #%d finished by signal with code: %d\n",
          child_i + 1, WTERMSIG(stat_val));
    } else if (WIFSTOPPED(stat_val)) {
      printf("parent talk : child process #%d finished stopped with code: %d\n",
             child_i + 1, WSTOPSIG(stat_val));
    }
  }

  printf("parent died : PID = %d ; PPID = %d ; GROUP = %d\n", getpid(),
         getppid(), getpgrp());
}
