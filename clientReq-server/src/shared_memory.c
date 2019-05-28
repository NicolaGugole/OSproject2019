#include <sys/shm.h>
#include <sys/stat.h>

#include "errExit.h"
#include "shared_memory.h"

 int alloc_shared_memory(key_t shmKey, size_t size) {
    // get, or create, a shared memory segment
    int shmid = shmget(shmKey, size, S_IRUSR | S_IWUSR | IPC_CREAT);
    if(shmid == -1)
      errExit("shmget fail");

    return shmid;
}

void *get_shared_memory(int shmid, int shmflg) {
    // attach the shared memory
    int *shmptr = (int *)shmat(shmid, NULL, shmflg);
    if(shmptr == (void *)-1)
      errExit("shmat fail");

    return shmptr;
}

void free_shared_memory(void *ptr_sh) {
    // detach the shared memory segments
    if(shmdt(ptr_sh) == -1)
      errExit("shmdt fail");
}

void remove_shared_memory(int shmid) {
    // delete the shared memory segment
    if(shmctl(shmid, IPC_RMID, NULL) == -1)
      errExit("shared memory removal fail");
}
