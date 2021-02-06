#ifndef TLS_H_
#define TLS_H_

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>

typedef struct thread_local_storage
{
    unsigned long tid;
    unsigned int size; /* size in bytes */
    unsigned int page_num; /* number of pages */
    struct page** pages; /* array of pointers to pages */
} TLS;

struct page {
    void* address; /* start address of page */
    int ref_count; /* counter for shared pages */
};


void tls_init();
void tls_protect(struct page* p);
void tls_unprotect(struct page* p);
int tls_create(unsigned int size);
int tls_write(unsigned int offset, unsigned int length, char* buffer);
int tls_read(unsigned int offset, unsigned int length, char* buffer);
int tls_destroy();
int tls_clone(pthread_t tid);

#endif 
