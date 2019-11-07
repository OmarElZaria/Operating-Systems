#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <unistd.h>
#include <ctype.h>
#include "common.h"
#include "wc.h"

//linked list structure used to store contents of hashtable
typedef struct LinkedList{
    int freq;
    char * string;
    struct LinkedList * next;
} list;

//wc is a structure which holds the hashtable and its size
struct wc {
	//you can define this struct to have whatever fields you want. 
   list** Table;
   long size;
}; 

unsigned int hash_function(struct wc* table, char* string){
    unsigned int hash = 0;
    
    for(; *string != '\0'; string++)
        hash ^= (hash << 5) + (hash >> 2) + *string;
    
    return hash % table->size;
}

//searches the hashtable
bool search_table(char* word, unsigned int hash, struct wc* table){
    list* current_list;
    
    for(current_list = table->Table[hash]; current_list != NULL; current_list = current_list->next){
        //increased frequency of word if detected in hashtable and return true if the desired word is found
        if(strcmp(word, current_list->string) == 0){
        
            current_list->freq += 1;
            
            return true;
        }           
    }
    
    return false;
}

//inserts contents into the hashtable
void insert(struct wc *wc, char* word){
    list* new_entry;
    
    unsigned int hash = hash_function(wc, word);
    
    //don't add word if already in table
    if(search_table(word, hash, wc))
        return;
    
    new_entry = malloc(sizeof(list));
    assert(new_entry);
    
    //assigns value to all parameters of new entry being inserted
    new_entry->freq = 1;
    new_entry->string = strdup(word);
    new_entry->next = wc->Table[hash];
    wc->Table[hash] = new_entry;
    
    return;
}

struct wc *
wc_init(char *word_array, long size)
{
	struct wc *wc;        

	wc = (struct wc *)malloc(sizeof(struct wc));
	assert(wc);
        
        //assign hash table size
        wc->size = 400000000;
        
        long i;
        char word[1024];
        int counter = 0;
        char character;
        
        if((wc->Table = malloc(sizeof(list *)* wc->size)) == NULL)
            return NULL;
        
        //initializing the hash table
        for(i = 0; i < wc->size; i++){
            wc->Table[i] = NULL;
        }
        
        for(i = 0; i < size; i++){
            character = word_array[i];
            if(!isspace(character)){
                word[counter] = character;
                counter++;
            }else{
                if(counter > 0){
                    word[counter] = 0;
                    insert(wc, word);
                    counter=0;
                }
            }
        }
        
        if(counter > 0){
            insert(wc, word);
        }
        
	return wc;
}

void wc_output(struct wc *wc){
    int i;
    list * current_list;

    if (!wc) return;

    for(i = 0; i < wc->size; i++) {
        current_list = wc->Table[i];
        while(current_list != NULL) {
            printf("%s:%d\n", current_list->string, current_list->freq);
            current_list = current_list->next;
        }
    }
}

void destroy(list * current_list){
    //destroys the linked list by free all its contents
    if(current_list->next == NULL)
    {
        free(current_list->string);
        free(current_list);
        return;
    }

    destroy(current_list->next);
    free(current_list->string);
    free(current_list);
    return;
}

void wc_destroy(struct wc *wc){
    long i;

    for(i = 0; i < wc->size; i++) {
        if(wc->Table[i] != NULL) {
            destroy(wc->Table[i]);
            wc->Table[i] = NULL;
        }
    }
    
    free(wc->Table);
    free(wc);
}
