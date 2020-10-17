/**
 * Ping IPC pipe test application, version 0.0.1
 * 
 * This application open pipe create process send data to this process and 
 * calculate sending time. 
 * To transfer started time to child process the shared memory used.
 * 
 * By Kirill Scherba™, 2019-01-24.
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <time.h>
#include <sys/time.h>

#include <sys/mman.h>

void* create_shared_memory(size_t size) {
  // Our memory buffer will be readable and writable:
  int protection = PROT_READ | PROT_WRITE;

  // The buffer will be shared (meaning other processes can access it), but
  // anonymous (meaning third-party processes cannot obtain an address for it),
  // so only this process and its children will be able to use it:
  int visibility = MAP_ANONYMOUS | MAP_SHARED;

  // The remaining parameters to `mmap()` are not important for this use case,
  // but the manpage for `mmap` explains their purpose.
  return mmap(NULL, size, protection, visibility, 0, 0);
}

const double NSECONDS = 1000000000.0;

double gettime()
{
   struct timespec ts;
   clock_gettime(CLOCK_REALTIME, &ts);

   return 1.00 * ts.tv_sec + ts.tv_nsec / NSECONDS;
}

int main(int argc, char** argv) {

    printf("Ping IPC pipe test application, ver. 0.0.1, (с) Kirill Scherba™, 2019\n");

    double *send_time =  (double *)create_shared_memory(sizeof(double));
    const char string[] = "Hello, world!\n";

    // Create pipe
    int fd[2];
    if(pipe(fd)) {
        perror("pipe creating");
        exit(1);
    }

    // Create process
    int pid;
    if((pid = fork()) == -1) {
        perror("process(fork) creating");
        exit(1);
    }

    // Run in child process
    else if(pid) {
        printf("Child process id = %d created\n", pid);

        // Child process closes up input side of pipe
        close(fd[0]);

        // Send "string" through the output side of pipe
        *send_time = gettime();
        int nbytes = write(fd[1], string, strlen(string)+1);
        printf("Child process write %d bytes to pipe\n", nbytes);

        sleep(1);
    }

    // Run in mine process
    else {
        printf("Main process id = %d running\n", getpid());

        char readbuffer[80];

        // Parent process closes up output side of pipe
        close(fd[1]);

        // Read in a string from the pipe
        int nbytes = read(fd[0], readbuffer, sizeof(readbuffer));
        double t = gettime() - *send_time;
        printf("Main process received %d bytes string: %s\n"
               "Send time: %.9f ms (%ld ns)\n",
               nbytes, readbuffer, t/1000.0, (long)(t * NSECONDS));

        sleep(1);
    }

    return (EXIT_SUCCESS);
}

