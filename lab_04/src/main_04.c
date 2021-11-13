#include <stdbool.h>   // false
#include <stdio.h>     // printf, fprintf, sleep
#include <stdlib.h>    // exit, NULL
#include <string.h>    // strlen
#include <sys/wait.h>  // wait
#include <unistd.h>    // fork, execlp, read, write, ssize_t, pipe

#define ERR_OK 0
#define ERR_FORK 1
#define ERR_EXEC 2
#define ERR_PIPE 3

#define MSG_CNT 2
#define CHILD_CNT 2
#define CHILD_SLP 2
#define BUFF_SZ 2048

int main() {
  int fd[2];
  char buffer[BUFF_SZ] = {0};
  int child_pids[CHILD_CNT] = {0};
  char* messages[MSG_CNT] = {
      "У окна дождь расскажет мне тайком, "
      "Как он жил вчера, как он будет жить потом. "
      "Я найду путь, который я искал, ",
      "Дождь возьмет меня туда, где луна висит меж скал. "};

  if (pipe(fd) == -1) {
    fprintf(stderr, "Can't pipe\n");
    exit(ERR_PIPE);
  }

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

      int msg_i = child_i % MSG_CNT;
      close(fd[0]);
      write(fd[1], messages[msg_i], strlen(messages[msg_i]));
      printf("child%u send : PID = %d ; MSG = %s\n", child_i, getpid(),
             messages[msg_i]);

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

  close(fd[1]);
  ssize_t readed = read(fd[0], buffer, BUFF_SZ);

  if (readed == -1) {
    printf("error on read\n");
  }

  printf("parent recv : %s\n", buffer);

  return 0;
}
