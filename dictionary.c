/**
 * dictionary.c
 *
 * Computer Science 50
 * Problem Set 5
 *
 * Implements a dictionary's functionality.
 */
#define _GNU_SOURCE // for rawmemchr - GNU extension
#include <sys/stat.h> // for stat struct
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // for memchr and strcmp
#include <stdint.h> // for murmur hash
#include "murmur3.h" // murmurhash 3
#include "dictionary.h"
#include <stdbool.h>
#include <pthread.h> // todo

// for bitwise OR to lowercase
#define BITS 32

// Some hash sizes I tried to get the fewest collisions
//#define HASH_SIZE 858546
//#define HASH_SIZE 1001637
//#define HASH_SIZE 1287819
//#define HASH_SIZE 1430910
//#define HASH_SIZE 5008185
#define HASH_SIZE 1287821 // try it with a prime


// this is the linked list node struct
typedef struct ChainLink
{
    uint32_t value;
    char key[45]; // longest word in English dictionary
    struct ChainLink* next;
} chain_link;

// Prototypes - create a new link, and add a link to an existing chain, free all
chain_link* CreateLink(uint32_t val, char * key);
chain_link* AddLink(chain_link* head, uint32_t val, char * key);
void destroy(chain_link* begin);


// Trav is a pointer to chain_link, used to "walk" the linked lists
chain_link *Trav;

// the actual hash table - yes, it's on the stack even though it's large
chain_link* hashtable[HASH_SIZE] = {NULL};

// seed value for murmurhash / 71 had fewest collisions
int seed = 71;
//47 = 9713
//2 = 9676
//3 = 9590
// 71 is main number

// counter is an integer that holds the number of words loaded from dictionary
unsigned int counter = 0;


/** This is the load function. It is called by speller.c and is the timed
 * function that loads words from the dictionary into whatever data structure
 * the programmer decides to implement. I chose hash table. Dictionary is
 * an unvalidated string to a path where the dictionary file is located.
 * 
 */
bool load (const char* dictionary)
{
    
    // hash is the return value from the murmur hash function
    // it may seem odd that it's a single element array, but that is because
    // I was switching back and forth between the 32 bit and 64 bit versions
    // and the 64 bit version returns 2 64 bit values.
    // It ends up the 32 bit version, though slightly slower, makes more sense
    // because the single value is faster to validate than both 64 bit values
    uint32_t hash[1];
    
    // the index in the hash table array where values will be stored
    int index = 0;
    
    // stat structure for getting information about the dictionary file
    struct stat test_stat;
    
    // the actual file to read from
    FILE* fp = fopen(dictionary, "r");
    if (fp == NULL)
    {
        //printf("Something went wrong.\n");
        return false;
    }
    
    // using stat to get the size, in bytes, of the dictionary file
    // so we know how much space to malloc and fread
    if (stat(dictionary, &test_stat)!= 0)
    {
        return false;
    }

    // mem_dict is the dictionary, loaded into memory - memory dictionary
    char *mem_dict;
    
    // allocate space for the dictionary
    if ((mem_dict = malloc((long long)test_stat.st_size + 1)) == NULL)
    {
        return false;
    }
    
    // load the whole dictionary in one shot to avoid disk IO slowness
    if (fread(mem_dict, test_stat.st_size, 1, fp) != 1)
    {
        return false;
    }
    
    // close the file
    fclose(fp);

    
    // this pair of pointers to char are used to navigate through mem_dict
    // and pick out individual words
    char *where_at;
    char *word_start = mem_dict; // word start will be address of each word
    
    // move the where_at pointer to the beginning of mem_dict
    where_at = mem_dict;
    
    // this loop is the guts of the load function, using the number of
    // bytes reported in the test_stat structure as the max condition
    while (where_at < mem_dict + test_stat.st_size)
    {
        // rawmemchar is a GNU extension - move pointer to first newline
        where_at = rawmemchr(where_at, '\n');
        
        // change the newline to a null terminator
        *where_at = '\0';
        
        // use word_start pointer to send string to hash function
        MurmurHash3_x86_32(word_start, (int)(where_at - word_start), seed, hash);
        
        // mod the return value from murmurhash to determine the index
        index = (hash[0]%HASH_SIZE);
        
        
        // make linked list nodes, either creating or adding based on
        // what is currently at the hashtable index
        if (hashtable[index] == NULL)
        {
            hashtable[index] = CreateLink(index, word_start);
        }
        else // this is an internal collision
        {
            hashtable[index] = AddLink(hashtable[index], index, word_start);
//            collisions++;
        }
        
        // move the pointers to the presumed start of the next word 
        where_at += 1;
        word_start = where_at;
        
        // increment the word count
        counter++;
    }
    
    // free the memory occupied by mem_dict
    free(mem_dict);
//    printf("Words: %d\n", counter);
//    printf("Collisions: %d\n", collisions);
    return true;
}


// create a new node for the linked list, inlined for performance
inline chain_link* CreateLink(uint32_t val, char * key)
{
    chain_link* temp = malloc(sizeof(chain_link));
    if (temp == NULL)
    {
        printf("Unable to allocate space.\n");
        exit(EXIT_FAILURE);
    }
    temp -> value = val;
    memcpy(temp -> key, key, strlen(key)+1);
    temp -> next = NULL;
    return temp;
}

// add a new node to an existing linked list, inlined for performance
inline chain_link* AddLink(chain_link* head, uint32_t val, char * key)
{
    chain_link* temp = malloc(sizeof(chain_link));
    if (temp == NULL)
    {
        printf("Unable to allocated space.\n");
        exit(EXIT_FAILURE);
    }
    temp -> value = val;
    memcpy(temp -> key, key, strlen(key)+1);
    temp -> next = head;
    return temp;
}


/**
 * Returns true if word is in dictionary else false.
 */
bool check(const char* word)
{
    int lenny = strlen(word);
    char temp[lenny+1];
//    printf("Arrived in check with word %s\n", word);
    for (int i = 0; i < lenny; i++)
    {
        temp[i] = word[i] | BITS; //;tolower(word[i]);
    }
    temp[lenny]='\0';
    // TODO
    //int seed = 71;
    uint32_t hash[1];
//    int collisions = 0;
    int index = 0;
    MurmurHash3_x86_32(temp, lenny, seed, hash);
    index = (hash[0]%HASH_SIZE);
    if (hashtable[index] == NULL)
    {
        return false;
        //hashtable[index] = CreateLink(index, word_start);
    }
    else // this is a collision
    {
        Trav = hashtable[index];
//        printf("%s", Trav -> key);
        while (Trav != NULL)
        {
            if (memcmp(temp, (Trav -> key), lenny) == 0)
            //if (strcmp(temp, (Trav -> key)) == 0)
            {
                return true;
            }
            else
            {
                Trav = Trav -> next;
            }
        }
        //hashtable[index] = AddLink(hashtable[index], index, word_start);
        //collisions++;
    }
    return false;
}


/**
 * Returns number of words in dictionary if loaded else 0 if not yet loaded.
 */
unsigned int size(void)
{
    return counter;
}


/**
 * Unloads dictionary from memory by calling the destroy() function
 * The unload function is a mandatory part of the project
 */
bool unload(void)
{
    for (int i = 0; i < HASH_SIZE; i++)
    {
        if (hashtable[i] != NULL)
        {
            destroy(hashtable[i]);
        }
    }
    return true;
}


/** destroy() is a recursive function that navigates linked lists and frees
 * each located node
 * 
 */
inline void destroy(chain_link* begin)
{
    if(begin == NULL)
    {
        return;
    }
    else
    {
        destroy(begin -> next);
        begin -> next = NULL;
        free(begin);
    }
}
