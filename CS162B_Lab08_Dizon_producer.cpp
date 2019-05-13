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
    ifstream input_file;
    input_file.open(argv[1]);
    if (not input_file) {
        cout << "Unable to open file";
        exit(1);
    }

    input_file.seekg(0, ios::end);
    unsigned int in_size = input_file.tellg();
    input_file.seekg(0, ios::beg);

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

    int shmSize = allocated_size;
    unsigned int uishmSize = shmSize;
    int shmSize2 = 0x2B;

    int shmFlags = IPC_CREAT | 0666;

    char* sharedMem;
    char* sharedMem2;

    shmId = shmget( shmKey, shmSize, shmFlags ); 
    shmId2 = shmget( shmKey2, shmSize2, shmFlags );

    char* com = new char[shmSize];
    if (input_file.is_open()) {
        while (in_size > 0) {
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
                printf( "semaphore++ in producer!\n" );
    
                sharedMem = (char*)shmat( shmId, NULL, 0 );
                sharedMem2 = (char*)shmat ( shmId2, NULL, 0);

                const char* w_command = "written";
                if( ((int*)sharedMem2) == (int*)-1 )
                {
                    perror( "shmop: shmat failed" );
                }
                else
                {
                    strcpy (com, sharedMem2);
                    if (not (strcmp(com,w_command) == 0)) {
                        strcpy( sharedMem2, w_command);
                    }
                }

                if( ((int*)sharedMem) == (int*)-1 )
                {
                    perror( "shmop: shmat failed" );
                }
                else
                {
                    if (not (strcmp(com,w_command) == 0)) {
                        cout << "bytes put into shared memory" << endl;
                        char* line = new char[min(uishmSize, in_size)];
                        input_file.read(line, min(uishmSize, in_size));
                        // cout << line << endl;
                        strcpy( sharedMem, line );  
                    }
                }

                if (not (strcmp(com,w_command) == 0)) in_size -= min(in_size, uishmSize);
                
                nOperations = 1;

                sema[0].sem_num = 0; 
                sema[0].sem_op = -1; 
                sema[0].sem_flg = SEM_UNDO | IPC_NOWAIT;

                opResult = semop( semId, sema, nOperations );
                if( opResult == -1 )
                {
                    perror( "semop (decrement)" );
                }
                else
                {
                    printf( "semaphore-- in producer!\n" );
                }
                sleep(1);
            }
            else
            {
                perror( "semop (increment)" );
            }
        }

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
                printf( "semaphore++ in producer!\n" );
        
                sharedMem = (char*)shmat( shmId, NULL, 0 );
                sharedMem2 = (char*)shmat ( shmId2, NULL, 0);

                if( ((int*)sharedMem2) == (int*)-1 )
                {
                    perror( "shmop: shmat failed" );
                }
                else
                {
                    strcpy (com, sharedMem2);
                    if (strcmp(com,"read") == 0) {
                        const char* d_command = "done";
                        strcpy( sharedMem2, d_command);
                    }  
                }

                nOperations = 1;

                sema[0].sem_num = 0; 
                sema[0].sem_op = -1; 
                sema[0].sem_flg = SEM_UNDO | IPC_NOWAIT;

                opResult = semop( semId, sema, nOperations );
                if( opResult == -1 )
                {
                    perror( "semop (decrement)" );
                }
                else
                {
                    printf( "semaphore-- in producer!\n" );
                }
                sleep(1);
            }
            else
            {
                perror( "semop (increment)" );
            }
        } while (not (strcmp(com,"read") == 0));
        
        input_file.close();
    }

    cout << "producer done" << endl;

    shmdt(sharedMem);
    shmdt(sharedMem2);

    shmctl(shmId, IPC_RMID, NULL);
    shmctl(shmId2, IPC_RMID, NULL);

    return 0;
}

