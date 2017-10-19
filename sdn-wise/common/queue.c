#include "queue.h"
#include  <stdlib.h>

/* OS includes */
#include "OSAL.h"
#include "OSAL_Tasks.h"
#include "OSAL_PwrMgr.h"

#include "msa.h"

Queue* queue_add_element(Queue* s, uint8_t* buffer, uint8_t rssi, uint8_t is_multicast)
{
  
  Message* p = osal_mem_alloc( 1 * sizeof(*p) );
  if( NULL == p )
  {
    return s;
  }
  
  p->buffer = osal_mem_alloc(1 * buffer[0]);
  if( NULL == p->buffer)
  {
    return s;
  }

  osal_memcpy(p->buffer,buffer,buffer[0]);  
  p->rssi = rssi;
  p->is_multicast = is_multicast;
  p->next = NULL;
  
  if( NULL == s )
  {
    osal_mem_free(p->buffer);
    osal_mem_free(p);
    return s;
  }
  else if( NULL == s->head && NULL == s->tail )
  {
    s->head = s->tail = p;
    s->size++;
    return s;
  }
  else if( NULL == s->head || NULL == s->tail )
  {
    osal_mem_free(p->buffer);
    osal_mem_free(p);
    return NULL;
  }
  else
  {
    s->tail->next = p;
    s->tail = p;
    s->size++;
    
    return s;
  }
}

Message* queue_remove_element(Queue* s)
{
  Message* h = NULL;
  Message* p = NULL;
  
  if( NULL == s )
  {
    return h;
  }
  else if( NULL == s->head && NULL == s->tail )
  {
    return h;
  }
  else if( NULL == s->head || NULL == s->tail )
  {
    return h;
  }
  
  h = s->head;
  p = h->next;
  s->head = p;
  if( NULL == s->head )  s->tail = s->head;   
  s->size--;
  return h;
}

Queue* queue_free(Queue* s)
{
  while( s->head )
  {
    queue_remove_element(s);
  }
  
  return s;
}

Queue* queue_new(void)
{
  Queue* p = osal_mem_alloc( 1 * sizeof(*p));
  
  if( NULL != p )
  {  
    p->head = p->tail = NULL;
    p->size = 0;
  }
  
  return p;
}


