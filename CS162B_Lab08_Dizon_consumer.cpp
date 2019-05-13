#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

using namespace std;

int main( int argc, char* argv[] )
{

    int allocated_size = atoi(argv[2]);

    ofstream output_file;
    output_file.open(argv[1]);

    int semId;

    key_t semKey = ftok("CS162B_Lab08_Dizon.txt", 1042069);

    int semFlag = IPC_CREAT | 0666;

    int nSems = 1;

    semId = semget( semKey, nSems, semFlag );
    if( semId == -1 )
    {
        perror( "semget" );
        exit( 1 );
    }

    int shmId;
    int shmId2;

    key_t shmKey = ftok("CS162B_Lab08_Dizon.txt", 69699);
    key_t shmKey2 = ftok("CS162B_Lab08_Dizon.txt", 42069);

    unsigned int shmSize = allocated_size;
    int shmSize2 = 0x2B;

    int shmFlags = IPC_CREAT | 0666;

    char* sharedMem;
    char* sharedMem2;

    shmId = shmget( shmKey, shmSize, shmFlags ); 
    shmId2 = shmget( shmKey2, shmSize2, shmFlags);

    char* r_command = new char[shmSize];
    do {
        int nOperations = 2;

        struct sembuf sema[nOperations];

        sema[0].sem_num = 0; // Use the first semaphore in the semaphore set
        sema[0].sem_op = 0; // Wait if semaphore != 0
        sema[0].sem_flg = SEM_UNDO; // See slides

        sema[1].sem_num = 0; // Use the first semaphore in the semaphore set
        sema[1].sem_op = 1; // Increment semaphore by 1
        sema[1].sem_flg = SEM_UNDO | IPC_NOWAIT; // See slides

        int opResult = semop( semId, sema, nOperations );

        if( opResult != -1 )
        {
            printf( "semaphore++ in consumer!\n" );

            sharedMem = (char*)shmat( shmId, NULL, 0 );
            sharedMem2 = (char*)shmat(shmId2, NULL, 0);

            if( ((int*)sharedMem2) == (int*)-1 )
            {
                perror( "shmop: shmat failed" );
            }
            else
            {
                strcpy( r_command, sharedMem2 );
                // cout << r_command << endl;
                const char* w_command = "read";
                if (not (strcmp(r_command,"done") == 0)) {
                    // cout << r_command << ' ' <<  (not (strcmp(r_command,"done") == 0)) << endl;
                    strcpy(sharedMem2, w_command);
                }
            }

            if( ((int*)sharedMem) == (int*)-1 )
            {
                perror( "shmop: shmat failed" );
            }
            else
            {
                if (strcmp(r_command,"written") == 0) {
                    cout << "bytes written into file" << endl;
                    char* line = new char[shmSize];
                    strcpy( line, sharedMem );
                    output_file << line;
                }
            }
            
            nOperations = 1;

            sema[0].sem_num = 0; // Use the first semaphore in the semaphore set
            sema[0].sem_op = -1; // Decrement semaphore by 1
            sema[0].sem_flg = SEM_UNDO | IPC_NOWAIT;

            opResult = semop( semId, sema, nOperations );
            if( opResult == -1 )
            {
                perror( "semop (decrement)" );
            }
            else
            {
                printf( "semaphore-- in consumer!\n" );
            }
            sleep(1);
        }
        else
        {
            perror( "semop (increment)" );
        }
    } while (not (strcmp(r_command,"done") == 0));
    
    output_file.close();

    shmdt(sharedMem);
    shmdt(sharedMem2);

    shmctl(shmId, IPC_RMID, NULL);
    shmctl(shmId2, IPC_RMID, NULL);

    return 0;
}

