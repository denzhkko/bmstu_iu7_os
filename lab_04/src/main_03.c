#include <stdbool.h>   // false
#include <stdio.h>     // printf, fprintf, sleep
#include <stdlib.h>    // exit, NULL
#include <sys/wait.h>  // wait
#include <unistd.h>    // fork, execlp

#define ERR_OK 0
#define ERR_FORK 1
#define ERR_EXEC 2

#define CMD_CNT 2
#define CHILD_CNT 2
#define CHILD_SLP 2
#define BUFF_SZ 2048

int main() {
  int child_pids[CHILD_CNT] = {0};
  char* cmds[CMD_CNT] = {"/home/deniska/dev/bmstu_iu7_oop/lab_03/build/lab_03",
                         "/home/deniska/dev/bmstu_iu7_cgcp/build/raytracing"};

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

      int cmd_i = child_i % CMD_CNT;

      int rc = execlp(cmds[cmd_i], cmds[cmd_i], (char*)NULL);

      if (rc == -1) {
        fprintf(stderr, "exec failed\n");
        exit(ERR_EXEC);
      }
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

  return 0;
}
