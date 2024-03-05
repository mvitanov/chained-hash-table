#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <semaphore.h>

  
/* 
 * 4KB shared memory segment
 * ---------------------------------------------------------------
 * Please note that any data bigger than 4KB passed to the segment
 * will be cut and only the first 4KB will be used!!!
 */
#define SHM_SIZE 4096 

/*
 * Name of the semaphore used to sync the writing to the shm
 * Creation and closing the semaphore is dealt with in the server
 */
#define SERVER_SEM_NAME "/chained_hash_table.sem"

void printUsageAndExit(char *executable) {
    fprintf(stderr, "Usage: %s --import -k <key> -v <value>\n", executable);
    fprintf(stderr, "%s --get -k <key>\n", executable);
    fprintf(stderr, "%s --delete -k <key>\n", executable);
    fprintf(stderr, "%s --shutdown\n", executable);
    exit(EXIT_FAILURE);
}
  
void writeToServer(char *shm, char *cmd, size_t cmd_length) {
    // open the named semaphore to synchronize correctly
    // sem_open will create the sempaphore in case it does not exist, 
    // but this should not happen if the client is already run
    sem_t *server_named_sem = sem_open(SERVER_SEM_NAME, O_CREAT, S_IRUSR | S_IWUSR, 1);
    if (server_named_sem == SEM_FAILED) {
        perror("ATTENTION: Client cannot open the named semaphore!");
        exit(EXIT_FAILURE);
    }

    // Wait on the named semaphore created by the server
    if (sem_wait(server_named_sem) == -1) {
        perror("ATTENTION: Error executiong sem_wait on the named server semaphore");
        exit(EXIT_FAILURE);
    }
    memcpy(shm, cmd, cmd_length);
    // Releasing the semaphore is done in the server, once the operation is completed
    
    // Close the named semaphore
    if (sem_close(server_named_sem) == -1) {
        perror("ATTENTION: Error when closing the named server semaphore!");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char **argv) {
    int isInsert = 0;
    int isGet = 0;
    int isDelete = 0;
    int shutDown = 0;
    char *key = NULL;
    char *value = NULL;

    // read options to configure the operation properly
    int long_option;
    static struct option long_options[] = {
        {"insert", no_argument, NULL, 'i'},
        {"get", no_argument, NULL, 'g'},
        {"delete", no_argument, NULL, 'd'},
        {"key", required_argument, NULL, 'k'},
        {"value", optional_argument, NULL, 'v'},
        {"shutdown", no_argument, NULL, 's'},
        {"help", no_argument, NULL, 'h'},
        {NULL, 0, NULL, 0}
    };

    while ((long_option = getopt_long(argc, argv, "igdk:v:h", long_options, NULL)) != -1) {
        switch (long_option) {
            case 'i':
                isInsert = 1;
                break;
            case 'g':
                isGet = 1;
                break;
            case 'd':
                isDelete = 1;
                break;
            case 'k':
                key = optarg;
                break;
            case 'v':
                value = optarg;
                break;
            case 's':
                shutDown = 1;
                break;
            case 'h':
                printUsageAndExit(argv[0]);
        }
    }

    if(isInsert + isGet + isDelete + shutDown < 1) {
        fprintf(stderr, "At least one of the arguments -i (--import), -g (--get) or -d (--delete) must be specified!");
        printUsageAndExit(argv[0]);
    } else if(isInsert + isGet + isDelete + shutDown > 1) {
        fprintf(stderr, "Only one of the arguments -i (--import), -g (--get) or -d (--delete) must be specified!");
        printUsageAndExit(argv[0]);
    }

    if(!shutDown && key == NULL) {
        fprintf(stderr, "Key parameter is required!\n");
        printUsageAndExit(argv[0]);
    }

    if(!shutDown && isInsert && value == NULL) {
        fprintf(stderr, "Value parameter is required when insert operation is used!\n");
        printUsageAndExit(argv[0]);
    }

    int shm_id;
    char *shm;

    // used for random number generation, seeding with time and process pid to avoid collisions
    srand((unsigned int) (getpid() * time(NULL)));
    // buffer for the random identifier
    char random_identifier[16];
    sprintf(random_identifier, "%d", rand());
  
    /*
     * We need to get the segment named "3723909", created by the server.
     */
    key_t shm_key = 3723909;
  
    if ((shm_id = shmget(shm_key, SHM_SIZE, 0644)) < 0) {
        fprintf(stderr, "Could not locate the shared memory segment for key %d!\n", shm_key);
        exit(EXIT_FAILURE);
    }
  
    if ((shm = shmat(shm_id, NULL, 0)) == (char *) -1) {
        perror("Attaching the memory segment to the data space failed!");
        exit(EXIT_FAILURE);
    }

    // we will always prepend the command with a random number
    // this will be used to differentiate in the server when one or more client processes 
    // overwrite the shm with the same content as before
    size_t cmd_length = strlen(random_identifier) + 1; // + newline
    // cmd regex: [igd]\nkey(\nvalue)?
    if(isInsert) {
        // composing the command in the form
        // i\nkey\nvalue
        // size is 1 cmd_letter + 1 newline + size of key + 1 newline + size of value + 1 terminal char
        cmd_length += 4 * sizeof(char) + strlen(key) + strlen(value);
        char cmd[cmd_length];

        strcpy(cmd, random_identifier);
        strcat(cmd, "\ni\n");
        strcat(cmd, key);
        strcat(cmd, "\n");
        strcat(cmd, value);

        writeToServer(shm, cmd, cmd_length);

        fprintf(stdout, "CMD: %s", cmd);
    } else if(isGet) {
        //composing the command in the form
        // g\nkey
        // size is 1 char cmd_letter + 1 char newline + size of key + 1 terminal char
        cmd_length += 3 + strlen(key);
        char cmd[cmd_length];

        strcpy(cmd, random_identifier);
        strcat(cmd, "\ng\n");
        strcat(cmd, key);

        writeToServer(shm, cmd, cmd_length);

        fprintf(stdout, "CMD: %s", cmd);
    } else if(isDelete) {
        //composing the command in the form
        // d\nkey
        // size is 1 char cmd_letter + 1 char newline + size of key + 1 terminal char
        cmd_length += 3 + strlen(key);
        char cmd[cmd_length];

        strcpy(cmd, random_identifier);
        strcat(cmd, "\nd\n");
        strcat(cmd, key);

        writeToServer(shm, cmd, cmd_length);

        fprintf(stdout, "CMD: %s", cmd);
    } else {
        char *cmd = "q\n";
        cmd_length = strlen(cmd) + 1;
        writeToServer(shm, cmd, cmd_length);

        printf("CMD: Shutdown Server\n");
    }
  
    if(shmdt(shm) != 0) {
        perror("Could not close memory segment!");
    }
  
    return EXIT_SUCCESS;
}