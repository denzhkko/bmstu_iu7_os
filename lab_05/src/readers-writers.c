#include <stdio.h>
#include <stdlib.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <wait.h>

#define ITER_CNT 20

#define READERS_CNT 5
#define WRITERS_CNT 3

#define ACTIVE_READERS 0
#define ACTIVE_WRITERS 1

#define WAITING_READERS 2
#define WAITING_WRITERS 3

#define MAX_WAIT_TIME 3

int
start_read(int sem_id)
{
  struct sembuf start_read[5] = {
    { WAITING_READERS, 1, 0 },  { ACTIVE_WRITERS, 0, 0 },
    { WAITING_WRITERS, 0, 0 },  { ACTIVE_READERS, 1, 0 },
    { WAITING_READERS, -1, 0 },
  };
  return semop(sem_id, start_read, 5) != -1;
}

int
stop_read(int sem_id)
{
  struct sembuf stop_read_sbuf[1] = {
    { ACTIVE_READERS, -1, 0 },
  };
  return semop(sem_id, stop_read_sbuf, 1) != -1;
}

int
start_write(int sem_id)
{
  struct sembuf start_write[5] = {
    { WAITING_WRITERS, 1, 0 },  { ACTIVE_READERS, 0, 0 },
    { ACTIVE_WRITERS, 0, 0 },   { ACTIVE_WRITERS, 1, 0 },
    { WAITING_WRITERS, -1, 0 },
  };
  return semop(sem_id, start_write, 5) != -1;
}

int
stop_write(int sem_id)
{
  struct sembuf stop_write[1] = {
    { ACTIVE_WRITERS, -1, 0 },
  };
  return semop(sem_id, stop_write, 1) != -1;
}

int
reader_run(int* const shcntr, const int sem_id, const int r_id)
{
  if (!shcntr) {
    return -1;
  }

  srand(time(NULL) + r_id);

  int stime;

  for (size_t i = 0; i < ITER_CNT; ++i) {
    stime = rand() % MAX_WAIT_TIME + 1;
    sleep(stime);

    if (!start_read(sem_id)) {
      perror("Ошибка start_read");
      exit(1);
    }

    int val = *shcntr;
    printf("Читатель #%d прочитал:  %3d : Время сна: %ds\n", r_id, val, stime);

    if (!stop_read(sem_id)) {
      perror("Ошибка stop_read");
      exit(1);
    }
  }

  return 0;
}

int
writer_run(int* const shcntr, const int sem_id, const int w_id)
{
  if (!shcntr) {
    return -1;
  }

  srand(time(NULL) + w_id + READERS_CNT);

  int stime;

  for (size_t i = 0; i < ITER_CNT; ++i) {
    stime = rand() % MAX_WAIT_TIME + 1;
    sleep(stime);

    if (!start_write(sem_id)) {
      perror("Ошибка start_write");
      exit(1);
    }

    int val = ++(*shcntr);
    printf("Писатель #%d записал:   %3d : Время сна: %ds\n", w_id, val, stime);

    if (!stop_write(sem_id)) {
      perror("Ошибка stop_write");
      exit(1);
    }
  }

  return 0;
}

typedef int (*foo_t)(int* const, const int, const int);

void
start_processes(foo_t fun, size_t count, int* const buff, const int sem_id)
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

  int fd = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | IPC_EXCL | perms);
  if (fd == -1) {
    perror("Ошибка работы shmget");
    return 1;
  }

  int* shcntr = shmat(fd, 0, 0);
  if (shcntr == (void*)-1) {
    perror("Ошибка работы shmat");
    return 1;
  }

  int sem_id = semget(IPC_PRIVATE, 4, IPC_CREAT | IPC_EXCL | perms);
  if (sem_id == -1) {
    perror("Ошибка работы semget");
    return 1;
  }

  if (semctl(sem_id, ACTIVE_READERS, SETVAL, 0) == -1 ||
      semctl(sem_id, ACTIVE_WRITERS, SETVAL, 0) == -1 ||
      semctl(sem_id, WAITING_WRITERS, SETVAL, 0) == -1 ||
      semctl(sem_id, WAITING_READERS, SETVAL, 0) == -1) {
    perror("Ошибка работы semctl");
    return 1;
  }

  int chpid;

  start_processes(reader_run, READERS_CNT, shcntr, sem_id);
  start_processes(writer_run, WRITERS_CNT, shcntr, sem_id);

  /*
  for (size_t i = 0; i < READERS_CNT; ++i) {
    chpid = fork();

    if (chpid == -1) {
      perror("Ошибка создания процесса читателя");
      exit(1);
    } else if (chpid == 0) {
      reader_run(shcntr, sem_id, i);
      return 0;
    }
  }

  for (size_t i = 0; i < WRITERS_CNT; ++i) {
    chpid = fork();

    if (chpid == -1) {
      perror("Ошибка создания процесса читателя");
      exit(1);
    } else if (chpid == 0) {
      writer_run(shcntr, sem_id, i);
      return 0;
    }
  }
  */

  for (size_t i = 0; i < WRITERS_CNT + READERS_CNT; ++i) {
    int status;
    if (wait(&status) == -1) {
      perror("Ошибка ожидания потомка");
      exit(1);
    }
  }

  if (shmdt((void*)shcntr) == -1 || shmctl(fd, IPC_RMID, NULL) == -1 ||
      semctl(sem_id, IPC_RMID, 0) == -1) {
    perror("Ошибка закрытия ресурсов");
    return 1;
  }

  return 0;
}
