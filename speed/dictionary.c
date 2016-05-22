/**
 * dictionary.c
 *
 * Computer Science 50
 * Problem Set 5
 *
 * Implements a dictionary's functionality.
 */
#define _GNU_SOURCE // for rawmemchr - GNU extension
//#include <sys/types.h>
#include <sys/stat.h> // for stat struct
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // for memchr and strcmp
#include <stdint.h> // for murmur hash
#include "murmur3.h" // murmurhash 3
#include <pthread.h>
#include <stdbool.h>
#include "dictionary.h"

//#include <limits.h>
//#include <ctype.h>
#define BITS 32

//#define HASH_SIZE 858546
//#define HASH_SIZE 1001637
//#define HASH_SIZE 1287819
//#define HASH_SIZE 1430910
//#define HASH_SIZE 5008185
#define HASH_SIZE 1287821 // try it with a prime
#define ACTUAL_SIZE 143091

typedef struct ChainLink
{
    uint32_t value;
    char key[45];
    struct ChainLink* next;
} chain_link;

chain_link* CreateLink(uint32_t val, const char * key);
chain_link* AddLink(chain_link* head, uint32_t val, const char * key);
// fstat example from fstat man page
// memchr example from memchr man page

//chain_link Start;
chain_link *Trav;
chain_link *UniTrav;
chain_link* hashtable[HASH_SIZE] = {NULL};

chain_link* unique_words[HASH_SIZE] = {NULL};
chain_link* unique_misspellings[HASH_SIZE] = {NULL};
unsigned int uniques = 0;

unsigned long actual[ACTUAL_SIZE] = {0};
int actual_counter = 0;


int seed = 71;
//47 = 9713
//2 = 9676
//3 = 9590
// 71 is main number

unsigned int counter = 0;
//int collisions = 0;
void destroy(chain_link* begin);

//int* hashes;

//int loader(void);


bool load (const char* dictionary)
{
    //int seed = 71;
    uint32_t hash[1];
    int index = 0;
    struct stat test_stat;
    
    FILE* fp = fopen(dictionary, "r");
    if (fp == NULL)
    {
        //printf("Something went wrong.\n");
        return false;
    }
    
    if (stat(dictionary, &test_stat)!= 0)
    {
        return false;
    }
//    printf("Size of chain_link: %zd\n", sizeof(chain_link));
//    printf("file size: %llu\n", (long long)test_stat.st_size);
    
    char *mem_dict;
    if ((mem_dict = malloc((long long)test_stat.st_size + 1)) == NULL)
    {
        return false;
    }
    
    if (fread(mem_dict, test_stat.st_size, 1, fp) != 1)
    {
        return false;
    }
    
    fclose(fp);

    char *where_at;

    char *word_start = mem_dict;
    
    
//    int counter = 0;
    where_at = mem_dict;
    while (where_at < mem_dict + test_stat.st_size)
    {
        where_at = rawmemchr(where_at, '\n');
        *where_at = '\0';
        MurmurHash3_x86_32(word_start, (int)(where_at - word_start), seed, hash);
        index = (hash[0]%HASH_SIZE);
        if (hashtable[index] == NULL)
        {
            hashtable[index] = CreateLink(index, word_start);
            actual[actual_counter] = index;
            actual_counter++;
        }
        else // this is a collision
        {
            hashtable[index] = AddLink(hashtable[index], index, word_start);
//            collisions++;
        }
        where_at += 1;
        word_start = where_at;
        counter++;
    }
    free(mem_dict);
    printf("Unique Locations: %d", actual_counter);
//    printf("Words: %d\n", counter);
//    printf("Collisions: %d\n", collisions);
    return true;
}

inline chain_link* CreateLink(uint32_t val, const char * key)
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

inline chain_link* AddLink(chain_link* head, uint32_t val, const char * key)
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
    if (hashtable[index] == NULL) // this is a misspelling
    {
        // add it to the reject array - increment uniques
        if (unique_words[index] == NULL) // have not seen this word before
        {
            unique_words[index] = CreateLink(index, word); // create a chainlink for it
            uniques++; // update uniques count
        }
        else // the slot in uniques is occupied - but is it the same word?
        {
            UniTrav = unique_words[index];
            bool found = false;
            while (UniTrav != NULL)
            {
                if (memcmp(word, (UniTrav -> key), lenny) == 0)
                {
                    found = true;
                    break;
                }
                else
                {
                    UniTrav = UniTrav -> next;
                }
            }
            if (found == false) // it was the same hash, but different word - update uniques
            {
                unique_words[index] = AddLink(unique_words[index], index, word);
                uniques++;
            }
        }
        return false;
        //hashtable[index] = CreateLink(index, word_start);
    }
    else // this is a collision with a dictionary word
    {
        Trav = hashtable[index];
//        printf("%s", Trav -> key);
        while (Trav != NULL)
        {
            if (memcmp(temp, (Trav -> key), lenny) == 0)
            //if (strcmp(temp, (Trav -> key)) == 0)
            {
                return true; // found it
            }
            else
            {
                Trav = Trav -> next; // keep looking
            }
        }
        //hashtable[index] = AddLink(hashtable[index], index, word_start);
        //collisions++;
    }
    return false;
}

/**
 * Loads dictionary into memory.  Returns true if successful else false.
 */
// bool load(const char* dictionary)
// {
//     if ( loader() != 0 )// TODO
//         return false;
//     else
//         return true;
// }

/**
 * Returns number of words in dictionary if loaded else 0 if not yet loaded.
 */
unsigned int size(void)
{
    // TODO
    printf("Unique Mispellings: %d\n", uniques);
    return counter;
}

/**
 * Unloads dictionary from memory.  Returns true if successful else false.
 */
bool unload(void)
{
    for (int i = 0; i < actual_counter; i++)
    {
        //actual[i];
        destroy(hashtable[actual[i]]);
    }
    // destroy the uniques table
    for (int i = 0; i < HASH_SIZE; i++)
    {
        if (unique_words[i] != NULL)
        {
            destroy(unique_words[i]);
        }
    }
    
    // PRE-ACTUAL IMPLEMENTATION
    // for (int i = 0; i < HASH_SIZE; i++)
    // {
    //     if (hashtable[i] != NULL)
    //     {
    //         destroy(hashtable[i]);
    //     }
    // }
    return true;
}

inline void destroy(chain_link* begin)
{
    if(begin == NULL)
    {
        return;
    }
    else
    {
        destroy(begin -> next);
//        printf("free called\n");
        begin -> next = NULL;
        free(begin);
    }
}
