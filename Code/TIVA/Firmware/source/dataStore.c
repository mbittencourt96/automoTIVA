/**************************************************************************/
/*!
  dataStore.c (Source)

  Implementation of functions to store and retrieve data of TIVA Flash memory, using a FIFO

  License: TBD

  By: Mariana Junghans, from UTFPR Curitiba
*/
/**************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/flash.h"

#include "dataStore.h"
#include "uart.h"


int isFull(Queue *Segments)
{
  return Segments->size == MAX_NUM_STRINGS;
}

char* FlashRead(uint32_t address)
{
    char* flashData = (char*) address;
    
    return flashData;
}

int isEmpty(Queue *Segments)
{
  return Segments->front == NULL;
}

void enqueue(Queue *Segments, char* data, uint32_t address)
{
  if(isFull(Segments)) {
        dequeue(Segments);
    }
  
  FlashSegment_t *newItem = createItem(data,address);
  
  if(isEmpty(Segments)) {
      Segments->front = newItem;
      Segments->rear = newItem;
  }
  else {
      Segments->rear->next = newItem;
      Segments->rear = newItem;
  }
    Segments->size++;
}

uint32_t dequeue(Queue *Segments)
{
  FlashSegment_t *temp = Segments->front;
  if(Segments->front == Segments->rear) {
      Segments->front = NULL;
      Segments->rear = NULL;
  }
  else {
      Segments->front = Segments->front->next;
  }
  
  uint32_t address = temp->address;
  FlashErase(address);
  free(temp);
  Segments->size--;
  return address;
}

FlashSegment_t* createItem(char* data, uint32_t address)
{
  FlashSegment_t *newItem = (FlashSegment_t*) malloc(sizeof(FlashSegment_t));
  newItem->data = data;
  newItem->address = address;
  newItem->next = NULL;
  return newItem;
}

void initQueue(Queue* Segments)
{
  Segments->front = NULL;
  Segments->rear = NULL;
  Segments->size = 0;
}

void eraseFlash(void)
{
  for (int k = 0; k < 10; k ++)
  {   
    FlashErase(FLASH_START_ADDRESS + FLASH_SEG_SIZE*k); //Erase the flash at specific address
  } 
}

//Function to search for a specific address in the queue

int searchQueue(Queue *Segments, uint32_t address) {
    if(isEmpty(Segments)) 
    {
        return 0;
    }
    FlashSegment_t *curr = Segments->front;
    while(curr != NULL) 
    {
      if(curr->address == address) 
      {
          return 1;            //value found
      }
      curr = curr->next;
    }
    return 0;                   //value not found
}

// Function to store a string in flash memory
bool storeStringInFlash(char *string, Queue* Segments) 
{
  
    int i;
    uint32_t address = FLASH_START_ADDRESS;
    uint32_t stringLength = strlen(string);
    
    div_t result = div(stringLength, 4);    //Divide by 4 to check how many int32 we have to program
    int number_parts = result.quot;          //The quotient is the number of parts we need to divide the string
    
    // Find the first free segment
    while (searchQueue(Segments,address) == 1)
    {
      if (i > 10)
      {
        address = dequeue(Segments);
        break;
      }
      address += FLASH_SEG_SIZE;
      i++;
    }
    
    enqueue(Segments,string,address);
    
    uint32_t* p = (uint32_t *) string;
    
    uint32_t initialAddress = address;
    
    for (int k = 0; k < number_parts; k++)
    {
      uint32_t* stringPart = p++;
      // Write part of the string (32 bits) to the current segment
      FlashProgram(stringPart, address, 4);
      address = address + 4;
    }
    
    uint32_t* remainingString = p;
    
    FlashProgram(remainingString, address, 4);
    
    // Verify the programmed data by reading it back from the Flash memory
    char* verifyBuffer;
   
    verifyBuffer = FlashRead(initialAddress);
    
    // Compare the original string with the verified string
    if (strcmp(string, verifyBuffer) == 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void printQueue(Queue *Segments) 
{
    if(isEmpty(Segments)) 
    {
        UARTSend(UART0_BASE,"Queue is empty\n",strlen("Queue is empty\n"));
        return;
    }
    
    FlashSegment_t *curr = Segments->front;
    char address_str[20];
    UARTSend(UART0_BASE,"Queue is:",strlen("Queue is:"));
    while(curr != NULL) {
        sprintf(address_str, "%X", curr->address);
        UARTSend(UART0_BASE,address_str,strlen(address_str));
        curr = curr->next;
    }
}