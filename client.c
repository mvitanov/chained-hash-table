#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <time.h>

  
/* 
 * 4KB shared memory segment
 * ---------------------------------------------------------------
 * Please note that any data bigger than 4KB passed to the segment
 * will be cut and only the first 4KB will be used!!!
 */
#define SHM_SIZE 4096 

void printUsageAndExit(char *executable) {
    fprintf(stderr, "Usage: %s --import -k <key> -v <value>\n", executable);
    fprintf(stderr, "%s --get -k <key>\n", executable);
    fprintf(stderr, "%s --delete -k <key>\n", executable);
    exit(EXIT_FAILURE);
}
  
int main(int argc, char **argv) {
    int isInsert = 0;
    int isGet = 0;
    int isDelete = 0;
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
            case 'h':
                printUsageAndExit(argv[0]);
        }
    }

    if(isInsert + isGet + isDelete < 1) {
        fprintf(stderr, "At least one of the arguments -i (--import), -g (--get) or -d (--delete) must be specified!");
        printUsageAndExit(argv[0]);
    } else if(isInsert + isGet + isDelete > 1) {
        fprintf(stderr, "Only one of the arguments -i (--import), -g (--get) or -d (--delete) must be specified!");
        printUsageAndExit(argv[0]);
    }

    if(key == NULL) {
        fprintf(stderr, "Key parameter is required!\n");
        printUsageAndExit(argv[0]);
    }

    if(isInsert == 1 && value == NULL) {
        fprintf(stderr, "Value parameter is required when insert operation is used!\n");
        printUsageAndExit(argv[0]);
    }

    int shm_id;
    char *shm;

    // used for random number generation
    srand((unsigned int)time(NULL));
    // TODO: rand generation should happen in a locked state to avoid collisions
    char random_identifier[16];
    sprintf(random_identifier, "%d", rand());
  
    /*
     * We need to get the segment named "3723909", created by the server.
     */
    key_t shm_key = 37239092;
  
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
        //composing the command in the form
        // i\nkey\nvalue
        // size is 1 cmd_letter + 1 newline + size of key + 1 newline + size of value + 1 terminal char
        cmd_length += 4 * sizeof(char) + strlen(key) + strlen(value);
        char cmd[cmd_length];

        strcpy(cmd, random_identifier);
        strcat(cmd, "\ni\n");
        strcat(cmd, key);
        strcat(cmd, "\n");
        strcat(cmd, value);

        memcpy(shm, cmd, cmd_length);
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

        memcpy(shm, cmd, cmd_length);

        fprintf(stdout, "CMD: %s", cmd);
    } else {
        //composing the command in the form
        // d\nkey
        // size is 1 char cmd_letter + 1 char newline + size of key + 1 terminal char
        cmd_length += 3 + strlen(key);
        char cmd[cmd_length];

        strcpy(cmd, random_identifier);
        strcat(cmd, "\nd\n");
        strcat(cmd, key);

        memcpy(shm, cmd, cmd_length);

        fprintf(stdout, "CMD: %s", cmd);
    }
  
    if(shmdt(shm) != 0) {
        perror("Could not close memory segment!");
    }
  
    return EXIT_SUCCESS;
}