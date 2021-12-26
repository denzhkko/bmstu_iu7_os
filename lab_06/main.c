#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#define READER_CNT 5
#define WRITER_CNT 3

#define ITERS 10

#define READ_TIMEOUT 300
#define WRITE_TIMEOUT 300
#define MAX_RAND_TIMEOUT 4000

HANDLE mutex;
HANDLE can_read;
HANDLE can_write;

LONG waiting_writers = 0;
LONG waiting_readers = 0;
LONG active_readers = 0;
bool active_writer = false;

int value = 0;

void
start_read()
{
  InterlockedIncrement(&waiting_readers);

  if (active_writer || WaitForSingleObject(can_write, 0) == WAIT_OBJECT_0) {
    WaitForSingleObject(can_read, INFINITE);
  }
  WaitForSingleObject(mutex, INFINITE);

  InterlockedDecrement(&waiting_readers);
  InterlockedIncrement(&active_readers);

  SetEvent(can_read);
  ReleaseMutex(mutex);
}

void
stop_read()
{
  InterlockedDecrement(&active_readers);

  if (active_readers == 0) {
    SetEvent(can_write);
  }
}

void
start_write(void)
{
  InterlockedIncrement(&waiting_writers);

  if (active_readers > 0 || active_writer) {
    WaitForSingleObject(can_write, INFINITE);
  }

  InterlockedDecrement(&waiting_writers);

  active_writer = true;
  ResetEvent(can_read);
}

void
stop_write(void)
{
  active_writer = false;

  if (WaitForSingleObject(can_read, 0) == WAIT_OBJECT_0) {
    SetEvent(can_read);
  } else {
    SetEvent(can_write);
  }
}

DWORD WINAPI
reader_run(CONST LPVOID lpParams)
{
  int reader_i = (int)lpParams;
  srand(time(NULL) + reader_i);

  for (int i = 0; i < ITERS; i++) {
    int stime = READ_TIMEOUT + rand() % MAX_RAND_TIMEOUT;
    Sleep(stime);

    start_read();
    printf("reader $%d:  %3d | sleep time: %dms\n", reader_i, value, stime);
    stop_read();
  }

  return 0;
}

DWORD WINAPI
writer_run(CONST LPVOID lpParams)
{
  int writer_i = (int)lpParams;
  srand(time(NULL) + writer_i + READER_CNT);

  for (int i = 0; i < ITERS; i++) {
    int stime = WRITE_TIMEOUT + rand() % MAX_RAND_TIMEOUT;
    Sleep(stime);

    start_write();
    ++value;
    printf("writer #%d: %3d | sleep time: %dms\n", writer_i, value, stime);
    stop_write();
  }
  return 0;
}

int
start_threads(HANDLE* threads, int th_cnt, LPTHREAD_START_ROUTINE routine)
{
  for (int i = 0; i < th_cnt; i++) {
    threads[i] = CreateThread(NULL, 0, routine, (LPVOID)i, 0, NULL);
    if (threads[i] == NULL) {
      perror("CreateThread");
      return 1;
    }
  }

  return 0;
}

int
main()
{
  setbuf(stdout, NULL);

  HANDLE readers_threads[READER_CNT];
  HANDLE writers_threads[WRITER_CNT];

  mutex = CreateMutex(NULL, FALSE, NULL);

  if (mutex == NULL) {
    perror("CreateMutex");
    return 1;
  }

  can_read = CreateEvent(NULL, FALSE, FALSE, NULL);
  can_write = CreateEvent(NULL, FALSE, FALSE, NULL);

  if (can_read == NULL || can_write == NULL) {
    perror("CreateEvent");
    return 1;
  }

  if (start_threads(readers_threads, READER_CNT, reader_run)) {
    perror("start_threads readers");
    return 1;
  }

  if (start_threads(writers_threads, WRITER_CNT, writer_run)) {
    perror("start_threads writers");
    return 1;
  }

  WaitForMultipleObjects(READER_CNT, readers_threads, TRUE, INFINITE);
  WaitForMultipleObjects(WRITER_CNT, writers_threads, TRUE, INFINITE);

  CloseHandle(can_write);
  CloseHandle(can_read);
  CloseHandle(mutex);

  return 0;
}
