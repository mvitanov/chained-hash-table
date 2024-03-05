# chained-hash-table

Server/client shm application

## Task description: 
Implement the following two programs:

### Server:

- Initializes a hash table of given size (provided via the command line)

- Supports the insertion of items in the hash table

- Hash table collisions are resolved by maintaining a linked list for each bucket/entry in the hash table

- Supports concurrent operations (multithreading) to perform insert, get and delete operations on the hash table

- Use readers-writer lock to ensure safety of concurrent operations

- Communicates with the client program using shared memory buffer (POSIX shm)

### Client:

Enqueues requests/operations (insert, read, delete) to the server (that will operate on the the hash table) via shared memory buffer (POSIX shm)

## Requirements

* Linux kernel (sync). Alternatively, Darwin can be used (not tested)
* GCC compiler

## How to run

1. Compile the server
    ```bash
    gcc server.c -o server -lrt -lpthread
    ```
2. Compile the client
    ```bash
    gcc client.c -o client -lrt -lpthread
    ```
3. Run the server by specifying also the size of the Hash Table
    ```bash
    ./server --size 20
    ```
4. Operations available with the client
    
    - Operation to insert (key, value) pair in the Hash Table
        ```bash
        ./client -i --key <key> --value <value>
        ```
    
    - Operation to delete a key from the Hash Table
        ```bash
        ./client -d --key <key>
        ```

    - Operation to retrieve the value of a key from the Hash Table
        ```bash
        ./client -g --key <key>
        ```

    - Operation to shutdown the server
        ```bash
        ./client --shutdown
        ```

## OS Tests
1. WSL (Windows 11) using the same gcc as the one in setup
2. Linux Kernel (Ubuntu 22.04.4 LTS) using the same gcc as the one in setup
3. Darwin (macOS Sonoma 14.3.1 with M2 Max) -> using the same gcc command as the one in the setup, but without the option lrt