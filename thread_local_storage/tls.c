#include "tls.h"

#define MAXTHREADS 128
int initialized; //variable to indicate whether our TLS is initialized yet. change to 1 when init.
int page_size;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; //need a mutex to avoid race conditions.

TLS** TLS_arr; //array to hold TLS for each thread.

static void tls_handle_page_fault(int sig, siginfo_t *si, void *context)
{
    //printf("handling sig fault! Address of sig fault: %x\n", ((unsigned int)si->si_addr));
    unsigned long* siginf;
    siginf = ((unsigned long*)si->si_addr);
    unsigned long p_fault = ((unsigned long)siginf) & ~(page_size - 1);
    
    //int index = -1; //track if we ever find the TLS.
    for (int i = 0; i < MAXTHREADS; i++)
    {
        for (int page = 0; page < TLS_arr[i]->page_num; page++)
        {
            //printf("comparing addresses: %x = %x ?\n", TLS_arr[i]->pages[page]->address, p_fault);
            if ((unsigned long) TLS_arr[i]->pages[page]->address == p_fault)
            {
                //printf("we exited the thread!\n");
                pthread_exit(NULL);
                //index = i;
            }
        }
    }
    //I think we don't need this if statement.
     signal(SIGSEGV, SIG_DFL);
     signal(SIGBUS, SIG_DFL);
     raise(sig);
}



void tls_init()
{
    
    struct sigaction sigact;
    /* get the size of a page */
    page_size = getpagesize();
    //printf("our page size: %d bytes\n", page_size);
    TLS_arr = (TLS**)calloc(MAXTHREADS, sizeof(TLS*));
    for (int i = 0; i < MAXTHREADS; i++)
    {
        TLS_arr[i] = (TLS*)calloc(1, sizeof(TLS));
    }
    /* install the signal handler for page faults (SIGSEGV, SIGBUS) */
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = SA_SIGINFO; /* use extended signal handling */
    sigact.sa_sigaction = tls_handle_page_fault;
    sigaction(SIGBUS, &sigact, NULL);
    sigaction(SIGSEGV, &sigact, NULL);
    
}

/*function that hashes into our TLS array. This is for speedier search and placement.
int hash_function(pthread_t tid)
{
    int hash = (tid/64)*13 % MAXTHREADS;
    printf("Our hash result: %d :%ld\n", hash, pthread_self());
    return hash;
    
}
*/
//protect a TLS after finished writing/reading to it.
void tls_protect(struct page* p)
{
    if (mprotect(p->address, page_size, 0)) {
        fprintf(stderr, "tls_protect: could not protect page. Errno:%d\n", errno);
        perror("Error: ");
        exit(1);
    }
    return;
}

//unprotect a TLS before writing/reading to it.
void tls_unprotect(struct page* p)
{
    if (mprotect(p->address, page_size,
        PROT_READ | PROT_WRITE) == -1) {
        fprintf(stderr, "tls_unprotect: could not unprotect page. errno:%d\n", errno);
        perror("Error: ");
        exit(1);
    }
    return;
}

//method to create TLS for our current thread. size is in bytes, but will allocate local storage
//to the page granularity. which is 4kb = 4096 bytes.
int tls_create(unsigned int size)
{
    pthread_mutex_lock(&mutex);
    if (!initialized){
        initialized = 1;
        tls_init();
    }

    int index = -1; //track if we ever find the TLS.
    for (int i = 0; i < MAXTHREADS; i++)
    {
        if (TLS_arr[i]->tid == (unsigned long)pthread_self())
        {
            index = i;
            break;
        }
    }

    if (index != -1) {
        printf("Current Thread already has TLS!\n");
        pthread_mutex_unlock(&mutex);
        return -1;
    }
    else if (size < 1) {
        pthread_mutex_unlock(&mutex);
        return -1;
    }

    int numPages = (size / page_size)+1;
    TLS* newTLS = calloc(1, sizeof(TLS));
    newTLS->tid = (unsigned long)pthread_self();
    newTLS->size = size;
    newTLS->page_num = numPages;
    newTLS->pages = calloc(numPages, sizeof(struct page*));
    //printf("thread %ld is trying to TLS create %d pages \n", pthread_self(), numPages);
    for (int i = 0; i < numPages; i++) {
        struct page* p = calloc(1, sizeof(struct page)); //need to calloc p since its a pointer.
        p->address = mmap(0, page_size, PROT_NONE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
        //printf("address of new page: %x\n", p->address);
        if (p->address == (void*)-1) { printf("mmap failed!\n"); }
        p->ref_count = 1;
        newTLS->pages[i] = p;
    }
    for (int index = 0; index < MAXTHREADS;  index++)
    {
        //find first index in TLS array that's free
        if (TLS_arr[index]->tid == 0)
        {
            //
            
            
            
            
            
            
            
            memcpy(&TLS_arr[index], &newTLS, sizeof(newTLS));
            TLS_arr[index] = newTLS;
            //memcpy(TLS_arr[index], newTLS, sizeof(newTLS));
            //printf("index %d of TLS_arr stores pthread: %x\n", index, TLS_arr[index]->tid);
            //TLS_arr[index] = newTLS;
            break;
        }
    }

    for (int i = 0; i < MAXTHREADS; i++)
    {
        if (TLS_arr[i]->tid == 0) {
            break;
        }        
    }
    pthread_mutex_unlock(&mutex);
    return 0;
}



int tls_write(unsigned int offset, unsigned int length, char* buffer)
{
    pthread_mutex_lock(&mutex);
   // printf("writing for thread %ls\n", pthread_self());
    if (initialized == 0) { pthread_mutex_unlock(&mutex); return -1; }//make sure we have initialized first.
    int index = -1; //track if we ever find the TLS.
    for (int i = 0; i < MAXTHREADS; i++)
    {       
        if (TLS_arr[i]->tid == (unsigned long) pthread_self())
        {
            index = i;
            break;
        }
    }

    //now error handle
    if (index == -1) {
        printf("Error: TLS not found for current thread.\n");
        pthread_mutex_unlock(&mutex);
        return -1;
    }
    else if (length < 1) {
        printf("Error: Write length must be greater than 0.!\n");
        pthread_mutex_unlock(&mutex);
        return -1;
    }
    else if ((offset + length) > TLS_arr[index]->size)
    {
        printf("Error: Write Address out of bounds.\n");
        pthread_mutex_unlock(&mutex);
        return -1;
    }


    //int first_page = offset / page_size; //find what pages we need to write to.
    //int last_page = (offset + length) / page_size;
    int count = 0;
    //write to pages. this is the inefficient implementation
    for (int memIndex = offset; memIndex < (offset + length); memIndex++)
    {
        struct page* p;
        unsigned int current_page = memIndex / page_size;
        //printf("writing to page: %d\n", current_page);
        unsigned int page_offset = memIndex % page_size;
        p = TLS_arr[index]->pages[current_page];
        //printf("this page is at address: %x\n", p);
        //if we have multiple threads using this page, clone it privately.
        if (p->ref_count > 1)
        {

            p->ref_count--; //make sure to reduce the ref_count.
            //printf("copying new page \n");
            //create new page, and copy what's in p to the new storage. 
            struct page* copy = calloc(1, sizeof(struct page));;
            copy->address = mmap(0, page_size, 0, MAP_ANON | MAP_PRIVATE, 0, 0);
            //printf("copy address is %x\n", copy->address);
            //printf("p address is %x\n", p->address);
            copy->ref_count = 1;
            TLS_arr[index]->pages[current_page] = copy; // address of tls_arr[index] is now copy.
            tls_unprotect(copy);
            tls_unprotect(p);
            memcpy(copy->address, p->address, page_size);//copy existing contents to copy page
            //printf("copy address is %x\n", copy->address);
            tls_protect(copy);
            tls_protect(p);
            
            p = TLS_arr[index]->pages[current_page]; //reassign p and continue.
        }
        
        tls_unprotect(p);
        char* dst; //make variable destination be a char pointer 
        dst = ((char*)p->address) + page_offset; //assign the address that destination points to correctly
        *dst = buffer[count];
        //printf("writing to index %x: %c\n", dst, *dst);
        tls_protect(p);
        count++;
    }
    pthread_mutex_unlock(&mutex);
    return 0;
}

int tls_read(unsigned int offset, unsigned int length, char* buffer)
{
    pthread_mutex_lock(&mutex);
    if (initialized == 0) {pthread_mutex_unlock(&mutex); return -1; }//make sure we have initialized first.
    int index = -1; //track if we ever find the TLS.
    for (int i = 0; i < MAXTHREADS; i++)
    {
        if (TLS_arr[i]->tid == (unsigned long)pthread_self())
        {
            index = i;
            break;
        }
    }
    //printf("beginning read\n");
    //now error handle
    if (index == -1) {
        printf("Error: TLS not found for current thread.\n");
        pthread_mutex_unlock(&mutex);
        return -1;
    }
    else if (length < 1) {
        printf("Error: read length must be greater than 0.\n");
        pthread_mutex_unlock(&mutex);
        return -1;
    }
    else if ((offset + length) > TLS_arr[index]->size)
    {
        printf("Error: Read Address out of bounds. Aborting...\n");
        pthread_mutex_unlock(&mutex);
        return -1;
    }
    //int first_page = offset / page_size; //find what pages we need to write to.
    //int last_page = (offset + length) / page_size;
    int count = 0;

    for (int memIndex = offset; memIndex < (offset + length); memIndex++)
    {
        struct page* p;
        unsigned int current_page = memIndex / page_size;
        unsigned int page_offset = memIndex % page_size;
        p = TLS_arr[index]->pages[current_page];
        //printf("address of page we're reading from: %x\n", p);
        char* source;
        tls_unprotect(p);
        source = ((char*)p->address) + page_offset;
        //printf("reading from index %x: %c\n", source, *source);
        buffer[count] = *source; //buffer[count] is the value that source is pointing to
        tls_protect(p);
        count++;
    }
    pthread_mutex_unlock(&mutex);
    return 0;
}

//frees tls for current thread
int tls_destroy()
{
    pthread_mutex_lock(&mutex);
    if (initialized == 0) { pthread_mutex_unlock(&mutex); return -1; }//make sure we have initialized first.

    int index = -1; //track if we ever find the TLS.
    for (int i = 0; i < MAXTHREADS; i++)
    {
        if (TLS_arr[i]->tid == (unsigned long)pthread_self())
        {
            index = i;
            break;
        }
    }

    if(index == -1){ pthread_mutex_unlock(&mutex); return -1; }
    //find where our current TLS is

    for (int i = 0; i < TLS_arr[index]->page_num; i++) //get each page and decrement usages.
    {
        //printf("freeing page: %d from thread %ld\n", i, pthread_self());
        struct page* p = TLS_arr[index]->pages[i]; 
        p->ref_count = p->ref_count - 1;
        //printf("the value of ref of TLS_arr after: %d\n", TLS_arr[index].pages[i]->ref_count);
        if (p->ref_count == 0) //if our ref_count is reduced to zero, free memory.
        {
            if (munmap(p->address, page_size) == -1)
            {
                printf("Munmap failed!\n");
            }
        }
    }
    TLS_arr[index]->tid = 0;

    pthread_mutex_unlock(&mutex);
	return 0;
}


int tls_clone(pthread_t tid)
{
    pthread_mutex_lock(&mutex);
    if (initialized == 0) { pthread_mutex_unlock(&mutex); return -1; }//make sure we have initialized first.
    int indexCur = -1; //track if we ever find the TLS.
    int indexJoin = -1;
    //find the TLS of the thread we're looking to clone.
    for (int i = 0; i < MAXTHREADS; i++)
    {
        if (TLS_arr[i]->tid == tid)
        {
            //printf("found thread we are cloning!\n");
            indexJoin = i;
        }
        else if (TLS_arr[i]->tid == pthread_self())
        {
            indexCur = i;
            break;//if our current thread has a tls, break.
        }
    }
    if (indexCur != -1 || indexJoin == -1)//if we have a tls already or other thread does, break.
    {
        printf("Error: TLS can't be found for target thread or TLS exists for current thread.\n");
        pthread_mutex_unlock(&mutex);
        return -1;
    }
    
    TLS* cloneTLS = TLS_arr[indexJoin];
    TLS* newTLS = calloc(1, sizeof(TLS));
    newTLS->tid = (unsigned long)pthread_self();
    newTLS->size = cloneTLS->size;
    newTLS->page_num = cloneTLS->page_num;
    newTLS->pages = calloc(newTLS->page_num, sizeof(struct page*));

    //update ref count so that when thread is deleted, we don't accidentally remove the TLS.
    for (int page = 0; page < newTLS->page_num; page++)
    {
        struct page* p = cloneTLS->pages[page];
        p->ref_count += 1;
        newTLS->pages[page] = p;
    }
    for (int index = 0; index < MAXTHREADS; index++)
    {
        //find first index in TLS array that's free
        if (TLS_arr[index]->tid == 0)
        {
            TLS_arr[index] = newTLS;
            //printf("index %d of TLS_arr stores pthread: %ld\n", index, TLS_arr[index]->tid);
            break;
        }
    }

    pthread_mutex_unlock(&mutex);
    return 0;
}
