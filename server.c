#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
  
/* 
 * 4KB shared memory segment
 * ---------------------------------------------------------------
 * Please note that any data bigger than 4KB passed to the segment
 * will be cut and only the first 4KB will be used!!!
 */
#define SHM_SIZE 4096 

/*
 * Structure for each node in the linked list used to implement a chained hash table
 *
 * According to the task description the table should support insertion of items
 * Therefore, the keys are values will accept generic types
 */
struct Node {
    void* key;
    // adding the dynamically allocated size as well, since void* cannot be deferenced in c
    size_t key_size;
    void* value;
    size_t value_size;
    struct Node* next;
};

/*
 * Structure for the chained hash table
 */
struct ChainedHashTable {
    size_t size;
    struct Node** table;
};

/*
 * Hash function for handling integer keys
 */
int hashInt(int* key, size_t size) {
    // TODO: make it better 
    return *key % size;
}

/*
 * Hash function for handling string keys
 */
int hashString(char* key, size_t size) {
    // TODO: make it better 
    int hash = 0;
    while(*key != '\0') {
        hash = (hash * 31) + *key;
        key++;
    }

    return hash % size;
}

/*
 * Helper function that delegates the correct hashing
 * 
 * Supports only integers and strings for now since this
 * can be input from the command line stdin
*/
int hash(void* key, size_t key_size, size_t table_size) {
    if(key_size == sizeof(int)) {
        return hashInt(key, table_size);
    } else {
        return hashString(key, table_size);
    }
}

/*
 * Helper function to initialize a new node with given key and value pair and their sizes
 */
struct Node* createNode(void* key, void* value, size_t key_size, size_t value_size) {
    // allocate the needed memory
    struct Node* newNode = (struct Node*)malloc(sizeof(struct Node));
    newNode->key = malloc(key_size);
    newNode->key_size = key_size;
    newNode->value = malloc(value_size);
    newNode->value_size = value_size;
    newNode->next = NULL;

    // copy key and value data to the node 
    memcpy(newNode->key, key, key_size);
    memcpy(newNode->value, value, value_size);

    return newNode;
}

/*
 * Helper function to deallocate the memory of a single node
 */
void freeNode(struct Node* node) {
    free(node->key);
    free(node->value);
    free(node);

    return;
}

/*
 * Helper function to initialize the chained hash table properly
 */
struct ChainedHashTable* initializeHashTable(size_t size) {
    struct ChainedHashTable* cht = (struct ChainedHashTable*)malloc(sizeof(struct ChainedHashTable));
    cht->size = size;
    cht->table = (struct Node**)malloc(size * sizeof(struct Node*));

    for (int i = 0; i < size; i++) {
        cht->table[i] = NULL;
    }

    return cht;
}


/*
 * Function to insert a key-value pair into the hash table
 * The keys and values can be generic data given by pointers and size
 */
void insert(struct ChainedHashTable* cht, void* key, void* value, size_t key_size, size_t value_size) {
    int index =  hash(key, key_size, cht->size);

    // Create a new node
    struct Node* newNode = createNode(key, value, key_size, value_size);
    
    // If the bucket is empty, insert the new node
    if (cht->table[index] == NULL) {
        cht->table[index] = newNode;
    } else {
        // If the bucket is not empty, check if the key exists in the linked list
        struct Node* current = cht->table[index];
        while(current != NULL) {
            if(memcmp(current->key, key, current->key_size) == 0) {
                current->value = (char *)realloc(current->value, value_size);
                memcpy(current->value, value, value_size);
                free(newNode);
                return;
            }

            current = current->next;
        }
        // If the list does not contain the key, add the new node to the front of the linked list
        newNode->next = cht->table[index];
        cht->table[index] = newNode;
    }
}

/*
 * Function to retrieve the value associated with a key from the hash table
 * The key size is also needed for the hashing and the correct reading of data from the pointer
 */
void* get(struct ChainedHashTable* cht, void* key, size_t key_size) {
    int index = hash(key, key_size, cht->size);
    
    // Traverse the linked list at the bucket to find the key
    struct Node* current = cht->table[index];
    while (current != NULL) {
        if (memcmp(current->key, key, key_size) == 0) {
            return current->value;
        }
        current = current->next;
    }
    
    // If key is not found
    return NULL;
}

/*
 * Function to delete a key-value pair from the hash table based on a key pointer and its size
 */
void delete(struct ChainedHashTable* cht, void* key, size_t key_size) {
    int index = hash(key, key_size, cht->size);
    
    // If the bucket is empty, there is nothing to delete
    if (cht->table[index] == NULL) {
        return;
    }
    
    // If the key to be deleted is in the first node of the linked list
    if (memcmp(cht->table[index]->key, key, key_size) == 0) {
        struct Node* temp = cht->table[index];
        cht->table[index] = cht->table[index]->next;
        free(temp->key);
        free(temp->value);
        free(temp);
        return;
    }
    
    // Traverse the linked list to find and delete the node with the specified key
    struct Node* current = cht->table[index];
    while (current->next != NULL && memcmp(current->next->key, key, key_size) != 0) {
        current = current->next;
    }

    if (current->next != NULL) {
        struct Node* temp = current->next;
        current->next = current->next->next;
        freeNode(temp);
    }
}

/*
 * Helper function to properly free the memory allocated for the hash table
 */
void freeHashTable(struct ChainedHashTable* cht) {
    for (int i = 0; i < cht->size; i++) {
        struct Node* current = cht->table[i];
        while (current != NULL) {
            struct Node* temp = current;
            current = current->next;
            freeNode(temp);
        }
    }
    free(cht->table);
    free(cht);
}

/*
 * Helper function to to conviniently print the hash table
 */
void printHashTable(struct ChainedHashTable* cht) {
    for (int i = 0; i < cht->size; i++) {
        printf("Bucket %d: ", i);
        struct Node* current = cht->table[i];
        while (current != NULL) {
            if(current->key_size == sizeof(int)) {
                printf("(%d", *(int*)current->key);
            } else {
                printf("(%s", (char*)current->key);
            }

            if(current->value_size == sizeof(int)) {
                printf(", %d) -> ", *(int*)current->value);
            } else {
                printf(", %s) -> ", (char*)current->value);
            }
            current = current->next;
        }
        printf("NULL\n");
    }
}

 
int main(int argc, char **argv) {
    int long_option;
    char *opt_value = NULL;
    static struct option long_options[] = {
        {"size", required_argument, NULL, 's'},
        {"help", no_argument, NULL, 'h'},
        {NULL, 0, NULL, 0}
    };

    while ((long_option = getopt_long(argc, argv, "s:h", long_options, NULL)) != -1) {
        switch (long_option) {
            case 's':
                opt_value = optarg;
                break;
            case 'h':
                fprintf(stderr, "Usage: %s -s <size>\n", argv[0]);
                fprintf(stderr, "%s --size <size>\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if(opt_value == NULL) {
        fprintf(stderr, "Size parameter is required.\n");
        fprintf(stderr, "Usage:\n %s -s <size>\n", argv[0]);
        fprintf(stderr, "%s --size <size>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Convert the size argument to an integer
    int table_size = atoi(opt_value);
    
    // Initialize the hash table
    struct ChainedHashTable* cht = initializeHashTable(table_size);

    char previous_content[SHM_SIZE];
    int shm_id;
    char *shm;  
  
    /*
     * Shared memory segment at 3723909
     */
    key_t shm_key = 37239092;
  
    
    if ((shm_id = shmget(shm_key, SHM_SIZE, IPC_CREAT | 0644)) < 0) {
        perror("Could not create a shared memory segment!");
        exit(EXIT_FAILURE);
    }
  
    if ((shm = shmat(shm_id, NULL, 0)) == (char *) -1) {
        perror("Attaching the memory segment to the data space failed!");
        exit(EXIT_FAILURE);
    }

    // in case there is something in the shared memory in the beginning -> ignore it
    strcpy(previous_content, shm);
  
    while (*shm != 'q'){
        //sleep(1);
        if(strcmp(shm, previous_content) == 0) {
            continue;
        }
        strcpy(previous_content, shm);

        // skip the dummy random number
        char *cmd = strchr(shm, '\n');
        if(cmd == NULL) {
            fprintf(stderr, "Invalid command: %s", shm);
            continue;
        } 
        cmd++; // the string points to \n

        char *operation_end = strchr(cmd, '\n');
        if(operation_end == NULL) {
            fprintf(stderr, "Invalid operation: %s", cmd);
            continue;
        } 
        
        size_t operation_size = operation_end - cmd + sizeof(char);
        char *operation = (char *)malloc(operation_size);
        memcpy(operation, cmd, operation_size - 1);
        operation[operation_size - 1] = '\0';

        char *key_value_string = ++operation_end;

        if(strcmp(operation, "i") == 0) {
            // retrieve key
            char *key_end = strchr(key_value_string, '\n');
            if(key_end == NULL) {
                fprintf(stderr, "Invalid key: %s", cmd);
                continue;
            } 

            size_t key_size = key_end - key_value_string + sizeof(char);
            char *key = (char *)malloc(key_size);
            memcpy(key, key_value_string, key_size - 1);
            key[key_size - 1] = '\0';

            fprintf(stdout, "key: %s\n", key);

            // retrieve value
            char *value = ++key_end;
            if(value == NULL) {
                fprintf(stderr, "Invalid value in command: %s", cmd);
                continue;
            }
            fprintf(stdout, "value: %s\n", value);
            fprintf(stdout, "keyLength: %zu\n", key_size);
            fprintf(stdout, "valueLength: %lu\n", strlen(value));
            insert(cht, key, value, key_size, strlen(value) + 1);

            free(key);
        } else if (strcmp(operation, "g") == 0 || strcmp(operation, "d") == 0) {
            // retrieve key
            char *key_end = strchr(key_value_string, '\n');
            char *key = NULL;
            int shouldFree = 0;
            if(key_end == NULL && key_value_string == NULL) {
                fprintf(stderr, "Invalid key in command: %s", cmd);
                continue;
            } else if(key_end == NULL) {
                key = key_value_string;
            } else {
                size_t key_size = key_end - key_value_string + sizeof(char);
                key = (char *)malloc(key_size);
                shouldFree = 1;

                memcpy(key, key_value_string, key_size - 1);
                key[key_size - 1] = '\0';
            }

            fprintf(stdout, "key: %s\n", key);
    
            if(*operation == 'g') {
                void* result = get(cht, key, strlen(key) + 1);
                printf("Result is: %s", (char*) result);
            } else {
                delete(cht, key, strlen(key) + 1);
            }

            if (shouldFree) {
                free(key);
            }
        } else {
            fprintf(stderr, "Invalid command: %s", cmd);
            continue;
        }

        printHashTable(cht);
        free(operation);
    }

    freeHashTable(cht);
  
    return EXIT_SUCCESS;
}