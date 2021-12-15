#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <wait.h>

#define SHM_SIZE 24
#define ITER_CNT 8

#define PRODUCER_CNT 3
#define CONSUMERS_CNT 3

#define BIN_SEM 0
#define BUF_FULL 1
#define BUF_EMPTY 2

#define MAX_SLEEP_TIME_P 2
#define MAX_SLEEP_TIME_C 5

typedef struct
{
  size_t consumers_i;
  size_t producers_i;
  char data[SHM_SIZE];
} shmbuf_t;

int
producer_run(shmbuf_t* const buf, const int sem_id, const int p_id)
{
  if (!buf) {
    return -1;
  }

  srand(time(NULL));

  int sleep_time;
  char ch;

  for (size_t i = 0; i < ITER_CNT; ++i) {
    sleep_time = rand() % MAX_SLEEP_TIME_P + 1;
    sleep(sleep_time);

    struct sembuf pl_sbuf[2] = { { BUF_EMPTY, -1, 0 }, { BIN_SEM, -1, 0 } };
    if (semop(sem_id, pl_sbuf, 2) == -1) {
      perror("Ошибка блокировки производителем");
      exit(1);
    }

    ch = 'a' + (char)(buf->producers_i % 26);

    buf->data[buf->producers_i] = ch;
    buf->producers_i = (buf->producers_i + 1) % SHM_SIZE;

    printf(
      "Производитель #%d запись: %c : Время сна: %ds\n", p_id, ch, sleep_time);

    struct sembuf pr_sbuf[2] = { { BUF_FULL, 1, 0 }, { BIN_SEM, 1, 0 } };
    if (semop(sem_id, pr_sbuf, 2) == -1) {
      perror("Ошибка освобождения производителем");
      exit(1);
    }
  }

  return 0;
}

int
consumer_run(shmbuf_t* const buf, const int sem_id, const int c_id)
{
  if (!buf) {
    return -1;
  }

  srand(time(NULL));

  int sleep_time;
  char ch;

  for (size_t i = 0; i < ITER_CNT; ++i) {
    sleep_time = rand() % MAX_SLEEP_TIME_C + 1;
    sleep(sleep_time);

    struct sembuf cl_sbuf[2] = { { BUF_FULL, -1, 0 }, { BIN_SEM, -1, 0 } };
    if (semop(sem_id, cl_sbuf, 2) == -1) {
      perror("Ошибка блокировки потребителем");
      exit(1);
    }

    ch = buf->data[buf->consumers_i];
    buf->consumers_i = (buf->consumers_i + 1) % SHM_SIZE;

    printf(
      "Потребитель   #%d чтение: %c : Время сна: %ds\n", c_id, ch, sleep_time);

    struct sembuf cr_sbuf[2] = { { BUF_EMPTY, 1, 0 }, { BIN_SEM, 1, 0 } };
    if (semop(sem_id, cr_sbuf, 2) == -1) {
      perror("Ошибка освобождения потребителем");
      exit(1);
    }
  }

  return 0;
}

typedef int (*foo_t)(shmbuf_t* const, const int, const int);

void
start_processes(foo_t fun, size_t count, shmbuf_t* const buff, const int sem_id)
{
  int chpid;

  for (size_t i = 0; i < count; ++i) {
    chpid = fork();

    if (chpid == -1) {
      perror("Ошибка создания процесса");
      exit(1);
    } else if (chpid == 0) {
      fun(buff, sem_id, i);
      return;
    }
  }
}

int
main()
{
  int perms = S_IRWXU | S_IRWXG | S_IRWXO;

  int fd = shmget(IPC_PRIVATE, sizeof(shmbuf_t), IPC_CREAT | IPC_EXCL | perms);
  if (fd == -1) {
    perror("Ошибка работы shmget");
    return 1;
  }

  shmbuf_t* buf = shmat(fd, 0, 0);
  if (buf == (void*)-1) {
    perror("Ошибка работы shmat");
    return 1;
  }

  memset(buf, 0, sizeof(shmbuf_t));

  int sem_id = semget(IPC_PRIVATE, 3, IPC_CREAT | IPC_EXCL | perms);
  if (sem_id == -1) {
    perror("Ошибка работы semget");
    return 1;
  }

  if (semctl(sem_id, BIN_SEM, SETVAL, 1) == -1 ||
      semctl(sem_id, BUF_EMPTY, SETVAL, SHM_SIZE) == -1 ||
      semctl(sem_id, BUF_FULL, SETVAL, 0) == -1) {
    perror("Ошибка работы semctl");
    return 1;
  }

  start_processes(producer_run, PRODUCER_CNT, buf, sem_id);
  start_processes(consumer_run, CONSUMERS_CNT, buf, sem_id);

  /*
  for (size_t i = 0; i < PRODUCER_CNT; ++i) {
    chpid = fork();

    if (chpid == -1) {
      perror("Ошибка создания процесса производителя");
      exit(1);
    } else if (chpid == 0) {
      producer_run(buf, sem_id, i);
      return 0;
    }
  }

  for (size_t i = 0; i < CONSUMERS_CNT; ++i) {
    chpid = fork();

    if (chpid == -1) {
      perror("Ошибка создания процесса потребителя");
      exit(1);
    } else if (chpid == 0) {
      consumer_run(buf, sem_id, i);
      return 0;
    }
  }
  */

  for (size_t i = 0; i < CONSUMERS_CNT + PRODUCER_CNT; ++i) {
    int status;
    if (wait(&status) == -1) {
      perror("Ошибка ожидания потомка");
      exit(1);
    }
  }

  if (shmdt((void*)buf) == -1 || shmctl(fd, IPC_RMID, NULL) == -1 ||
      semctl(sem_id, IPC_RMID, 0) == -1) {
    perror("Ошибка закрытия ресурсов");
    return 1;
  }

  return 0;
}
