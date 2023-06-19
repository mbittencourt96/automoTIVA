/**************************************************************************/
/*!
  dataStore.h
  
  Definition of functions to store and retrieve data of TIVA Flash memory

  License: GNU General Public License v3.0

  By: Mariana Junghans, from UTFPR Curitiba
*/
/**************************************************************************/


#ifndef _DATASTORE_H_
#define _DATASTORE_H_

#define FLASH_SEG_SIZE 1024 // Size of each flash segment
#define MAX_NUM_STRINGS 10  // Maximum number of strings to store
#define STRING_SIZE 64      // Maximum size of each string

// Start address of the flash segment
#define FLASH_START_ADDRESS ((uint32_t)0x0003F000)

// Data structure that represents a flash segment
typedef struct FlashSegment_t {
    char* data;
    uint32_t address;
    struct FlashSegment_t* next;
    struct FlashSegment_t* prev;
} FlashSegment_t;

typedef struct 
    {
      FlashSegment_t *rear;
      FlashSegment_t *front;
      int size;
    } Queue;

int isFull(Queue *Segments);
int isEmpty(Queue *Segments);
void enqueue(Queue *Segments, char* data, uint32_t address);
uint32_t dequeue(Queue *Segments);
FlashSegment_t* createItem(char* data, uint32_t address);
Queue* eraseFlash(void);
char* FlashRead(uint32_t address);
void initQueue(Queue* Segments);
bool storeStringInFlash(char *string, Queue* Segments);
void printQueue(Queue *Segments);

























#endif