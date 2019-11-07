#include <assert.h>
#include <stdlib.h>
#include <ucontext.h>
#include <stdbool.h>
#include "thread.h"
#include "interrupt.h"
#define MAX_SIZE THREAD_MAX_THREADS

/* This is the wait queue structure */
struct wait_queue {
	/* ... Fill this in Lab 3 ... */
    struct thread* wait;
    struct wait_queue* next;
};

/* This is the thread control block */
struct thread {
	/* ... Fill this in ... */
    Tid id_thread;
    void* stack;
    struct ucontext context_thread;
    struct wait_queue* queue_wait;
};


//this structure is a liinked list that allows me to keep track of all my ready/exit queues
struct queue_list{
    struct thread* in_queue;
    struct queue_list* next;
};

int tids[MAX_SIZE];
struct queue_list* queue_exit;
struct queue_list* queue_ready;
struct thread* current_thread;

//this function insert a new ready/exit queue into my linked list of queues
void insert_queue(struct queue_list*  current, struct queue_list*  previous, struct thread*  current_thread);

void insert_wait_queue(struct wait_queue*  current, struct wait_queue*  previous, struct thread* sleep_thread);

//this function allows me to get the context of an old thread and set the context for the new thread to start running
bool get_and_set_context(struct thread* old_thread,struct thread* new_thread);


struct thread* hellaDeepSearch(struct wait_queue* wq, Tid tid)
{
    struct thread* found = NULL;
    struct wait_queue* current = wq->next;
    while(current!=NULL && found==NULL) {
        if(current->wait->id_thread == tid) found = current->wait;
        else found = hellaDeepSearch(current->wait->queue_wait,tid);
        
        current = current->next;
    }
    return found;
}
/* thread starts by calling thread_stub. The arguments to thread_stub are the
 * thread_main() function, and one argument to the thread_main() function. */
void
thread_stub(void (*thread_main)(void *), void *arg)
{   
    interrupts_on();
    thread_main(arg); // call thread_main() function with arg
    thread_exit();
    exit(0);
}

void
thread_init(void)
{
	/* your optional code here */
    
    //initialize my table of ids to all available spaces except for thread 0
    int i;
    
    for(i = 0; i < MAX_SIZE; i++){
        tids[i] = 0;
    }
    tids[0] = 1;
    
    //allocate memomery fo all my other global variables
    queue_exit = (struct queue_list*)malloc(sizeof(struct queue_list));
    queue_exit->in_queue = NULL;
    queue_exit->next = NULL;
    
    
    queue_ready = (struct queue_list*)malloc(sizeof(struct queue_list));
    queue_ready->in_queue = NULL;
    queue_ready->next = NULL;  
    
    current_thread = (struct thread*)malloc(sizeof(struct thread));
    current_thread->id_thread = 0;
    current_thread->queue_wait = wait_queue_create();
}

Tid
thread_id()
{
	return current_thread->id_thread;
}

Tid
thread_create(void (*fn) (void *), void *parg)
{
    int enable = interrupts_off();
    
    int new_id = 0;
    for(new_id =0; tids[new_id] == 1; new_id++){
        if((MAX_SIZE-1) == new_id){
            interrupts_set(enable);
            return THREAD_NOMORE;
        }
    }
    
    struct queue_list* current = queue_ready->next;
    struct queue_list* previous = queue_ready;
  
    struct thread* new_thread = (struct thread*)malloc(sizeof(struct thread));
    
    if(new_thread == NULL){ 
        interrupts_set(enable);
        return THREAD_NOMEMORY;
    }
    void* stack_pointer = malloc(THREAD_MIN_STACK+24);
    
    if(stack_pointer == NULL){
        free(new_thread);
        interrupts_set(enable);
        return THREAD_NOMEMORY; 
    }
    stack_pointer = stack_pointer + THREAD_MIN_STACK +24;
    
    getcontext(&new_thread->context_thread);
    
    //assign values to registers and stack for the new thread created
    new_thread->context_thread.uc_mcontext.gregs[REG_RIP] = (unsigned long)&thread_stub;
    new_thread->context_thread.uc_mcontext.gregs[REG_RSP] = (unsigned long)stack_pointer; 
    new_thread->context_thread.uc_mcontext.gregs[REG_RSI] = (unsigned long)parg;
    new_thread->context_thread.uc_mcontext.gregs[REG_RDI] = (unsigned long)fn; 
    
    new_thread->context_thread.uc_stack.ss_sp = stack_pointer - (THREAD_MIN_STACK + 24);
    new_thread->context_thread.uc_stack.ss_size = THREAD_MIN_STACK + 24;
    new_thread->context_thread.uc_stack.ss_flags = 0;
    
    new_thread->stack = stack_pointer - (THREAD_MIN_STACK + 24);
    
    //fill a space in my table of ids for my new thread
    new_thread->id_thread = new_id;
    new_thread->queue_wait = wait_queue_create();
    tids[new_id] = 1;
    
    insert_queue(current, previous, new_thread);
    
    interrupts_set(enable);
    return new_thread->id_thread;
}


Tid
thread_yield(Tid want_tid)
{    
    int enable = interrupts_off();
    //allows thread to yield itself
    if(current_thread->id_thread == want_tid || want_tid == THREAD_SELF){
        interrupts_set(enable);
        return current_thread->id_thread;
    }else if(want_tid == THREAD_ANY){
        
        if(queue_ready->next != NULL){
            
            //parses through my ready_queue to find a spot for the new thread that will be yielded
            struct thread* old_thread;
            old_thread = current_thread;
            
            struct thread* new_thread;
            new_thread = queue_ready->next->in_queue;
            struct queue_list*  current = queue_ready->next;

            queue_ready->next = queue_ready->next->next; 
            
            current->in_queue = NULL;
            free(current);
            
            current = queue_ready->next;
            struct queue_list*  previous = queue_ready;;
            
            insert_queue(current, previous, old_thread);
            
            //this is the running thread now
            current_thread = new_thread;
        
            if(get_and_set_context(old_thread, new_thread)){   
                //free all memory in my exit queues after a yield occurs
                int enable2 = interrupts_off();
                if(queue_exit->next != NULL){
                    struct queue_list* temp;
                    while(queue_exit->next != NULL){
                        temp = queue_exit->next;

                        if(temp->in_queue->id_thread == 0){
                            //move to next queue if it doesn't hold anything
                            queue_exit->next = temp->next;
                            free(temp);
                        }else{
                            //free all allocated memory
                            free(temp->in_queue->stack);
                            wait_queue_destroy(temp->in_queue->queue_wait);
                            free(temp->in_queue);
                            queue_exit->next = temp->next;
                    
                            free(temp);
                        } 
                    }
                }
                interrupts_set(enable2);
                interrupts_set(enable);
                return new_thread->id_thread;
            }
            
        }else{
            interrupts_set(enable);
            return THREAD_NONE;
        }
    
    }else{
        if(want_tid < -2 || want_tid >= MAX_SIZE){
            interrupts_set(enable);
            return THREAD_INVALID;
    
        }if(tids[want_tid] == 0 && want_tid >= 0 && want_tid < MAX_SIZE){
            interrupts_set(enable);
            return THREAD_INVALID;
    
        }if(queue_ready->next != NULL){
            struct queue_list* current = queue_ready->next;
            struct queue_list* previous = queue_ready;
            
            bool key = false;

            //this searches for the ready queue to find a queue that already exists which holds the id of the current thread running
            while(!key && current!=NULL){
                if(current->in_queue->id_thread != want_tid){
                    previous = current;
                    current = current->next;
                }else{

                    previous->next = current->next;
                    key = true;

                }
            }
            if(!key){
                interrupts_set(enable);
                return THREAD_INVALID;
            }
            
            struct thread* old_thread;
            old_thread = current_thread;

            struct thread* new_thread;
            new_thread = current->in_queue;
            
            current->in_queue = NULL;
            free(current);
            
            current = queue_ready->next;
            previous = queue_ready;

            insert_queue(current, previous, old_thread);
            
            //this is the running thread now
            current_thread = new_thread;
            
            if(get_and_set_context(old_thread, new_thread))  {    
                //free all memory in my exit queues after a yield occurs
                int enable2 = interrupts_off();
                if(queue_exit->next != NULL){
                    struct queue_list* temp;
                    while(queue_exit->next != NULL){
                        temp = queue_exit->next;

                        if(temp->in_queue->id_thread == 0){
                            //move to next queue if it doesn't hold anything
                            queue_exit->next = temp->next;
                            free(temp);
                        }else{
                            //free all allocated memory
                            free(temp->in_queue->stack);
                            wait_queue_destroy(temp->in_queue->queue_wait);
                            free(temp->in_queue);
                            queue_exit->next = temp->next;

                            free(temp);
                        }    
                    }
                }
                interrupts_set(enable2);
                interrupts_set(enable);
                return new_thread->id_thread;
            }

        }else{
            interrupts_set(enable);
            return THREAD_INVALID;
        }
    }
    interrupts_set(enable);
    return THREAD_FAILED;
}

void
thread_exit()
{
    int enable = interrupts_off();
    
    // throw wait queue into ready queue
    thread_wakeup(current_thread->queue_wait,1);
    
    if(queue_ready->next != NULL){
        
        //when the ready queue is not empty, I add unwanted threads to my exit queue
        struct thread* exit_thread = current_thread;
        struct thread* new_thread = queue_ready->next->in_queue;
        
        //move on to the next queue in my list of ready queues
        struct queue_list* current = queue_ready->next;

        queue_ready->next = queue_ready->next->next; 
        free(current);
        
        //add to the exit queue        
        current = queue_exit->next;
        struct queue_list* previous = queue_exit;
        
        insert_queue(current, previous, exit_thread);       
        
        current_thread = new_thread;
        
        //allocate a free space in the table of thread ids
        tids[exit_thread->id_thread] = 0;

        setcontext(&new_thread->context_thread);
        interrupts_set(enable);
    }else{
        //this goes through all my exit queues and free any memory I allocated previously
        if(queue_exit->next != NULL){
            struct queue_list* temp;
            while(queue_exit->next != NULL){
                temp = queue_exit->next;
                
                //check if the queue holds something or not
                if(temp->in_queue->id_thread == 0){
                    //move to next queue if it doesn't hold anything
                    queue_exit->next = temp->next;
                    free(temp);
                }else{
                    //free all allocated memory
                    free(temp->in_queue->stack);
                    wait_queue_destroy(temp->in_queue->queue_wait);
                    free(temp->in_queue);
                    queue_exit->next = temp->next;
                    
                    free(temp);
                } 
            }
        }
        
        interrupts_set(enable);
        exit(0);
    }
    
    interrupts_set(enable);
}

Tid
thread_kill(Tid tid)
{
    int enable = interrupts_off();
    if (queue_ready->next != NULL){

        
        //struct queue_list* current = queue_ready->next;
        //struct queue_list* previous = queue_ready;
        //struct thread* kill_thread;
        
        struct thread* found_thread = NULL;
        struct queue_list* current = queue_ready->next;
        while(current!=NULL && found_thread==NULL) {
            if(current->in_queue->id_thread == tid) found_thread = current->in_queue;
            else found_thread = hellaDeepSearch(current->in_queue->queue_wait,tid);
        
            current = current->next;
        }
        if(found_thread == NULL){
            interrupts_set(enable);
            return THREAD_INVALID;
        }

/*
        bool key = false; 
        
        //this searches for the ready queue to find a queue that already exists which holds the id of the current thread running
        while(!key && current!=NULL){
            if(current->in_queue->id_thread != tid){
                
                previous = current;
                current = current->next;
            }
            else{
                
                previous->next = current->next;
                key = true;
                
                kill_thread = current->in_queue;
 
                //this allows me to delete the ready queue before moving it to the exit queue
                free(current);                
            }
        }
        if(!key){

            if(current_thread->queue_wait == NULL){
                interrupts_set(enable);
                return THREAD_INVALID;
            }
            
            if(current_thread->queue_wait->next == NULL){
                if(current_thread->queue_wait->next->wait->id_thread == tid){
                    key = true;
                    
                    kill_thread = current_thread->queue_wait->next->wait;
                    
                    free(current_thread->queue_wait->next);
                }else{
                    interrupts_set(enable);
                    return THREAD_INVALID;
                }
            }else{
                    struct thread* found = NULL;
                    struct wait_queue* current_wait = current->in_queue->queue_wait->next;
                    while(current!=NULL && found==NULL) {
                        if(current->wait->id_thread == tid) found = current->wait;
                        else found = hellaDeepSearch(current->wait->queue_wait,tid);

                        current = current->next;
                    }

                struct wait_queue* current_wait = current_thread->queue_wait->next;
                struct wait_queue* previous_wait = current_thread->queue_wait;
                while(current_wait != NULL && !key){
                    if(current_wait->wait->id_thread != tid){
                        previous_wait = current_wait;
                        current_wait = previous_wait;
                    }else{
                        previous_wait->next = current_wait->next;
                        key = true;

                        kill_thread = current_wait->wait;

                       //this allows me to delete the ready queue before moving it to the exit queue
                        free(current_wait);
                    }
                }

                if(found == NULL){

                    interrupts_set(enable);
                    return THREAD_INVALID;

                }
            }

        }*/
        free(current);
        //add to my exit queue after thread is killed
        current = queue_exit->next;
        struct queue_list* previous = queue_exit;
        
        insert_queue(current, previous, found_thread);       
        
        //allocate space in my table of ids
        tids[found_thread->id_thread] = 0;
        
        interrupts_set(enable);
        return found_thread->id_thread;    
    }else{
        interrupts_set(enable);
        return THREAD_INVALID;
    }
    
    interrupts_set(enable);
    return THREAD_FAILED;
}


void insert_queue(struct queue_list*  current, struct queue_list*  previous, struct thread* current_thread){
    int enable = interrupts_off();
    struct queue_list* new_queue = (struct queue_list*) malloc(sizeof(struct queue_list));
    
    //moves to end of list
    while(current!=NULL){
        previous = current;
        current = current->next;
    }
  
    //inserts the queue at end of list
    previous->next = new_queue;
    new_queue->next = NULL;
    new_queue->in_queue = current_thread;
    interrupts_set(enable);
}

bool get_and_set_context(struct thread* old_thread, struct thread* new_thread){
    volatile bool context = false;
    getcontext(&old_thread->context_thread);

    if(context){
        return true;
    }
    
    context = true; 
    setcontext(&new_thread->context_thread);
    return false;
}

void insert_wait_queue(struct wait_queue*  current, struct wait_queue*  previous, struct thread* sleep_thread){
    int enable = interrupts_off();
    struct wait_queue* new_queue = (struct wait_queue*) malloc(sizeof(struct wait_queue));
    
    //moves to end of list
    while(current!=NULL){
        previous = current;
        current = current->next;
    }
  
    //inserts the queue at end of list
    previous->next = new_queue;
    new_queue->next = NULL;
    new_queue->wait = sleep_thread;
    interrupts_set(enable);
}



/*******************************************************************
 * Important: The rest of the code should be implemented in Lab 3. *
 *******************************************************************/

/* make sure to fill the wait_queue structure defined above */
struct wait_queue *
wait_queue_create()
{
    int enable = interrupts_off(); 
    struct wait_queue *wq;

    wq = malloc(sizeof(struct wait_queue));
    assert(wq);

    wq->wait = NULL;
    wq->next = NULL;

    interrupts_set(enable);
    return wq;
}


void
wait_queue_destroy(struct wait_queue *wq)
{
    int enable = interrupts_off();
    
    if(wq->next == NULL){
        free(wq);
        wq = NULL;
        interrupts_set(enable);
    }
    
    thread_wakeup(wq, 1);
}

Tid
thread_sleep(struct wait_queue *queue)
{
    int enable = interrupts_off();
    
    if(queue == NULL){
        interrupts_set(enable);
        return THREAD_INVALID;
    }
    
    if(queue_ready->next != NULL){
        
        struct thread* old_thread = current_thread;
        struct thread* new_thread = queue_ready->next->in_queue;
        
        struct queue_list* current = queue_ready->next;

        queue_ready->next = queue_ready->next->next; 
        current->in_queue = NULL;
        free(current);
        
        struct wait_queue* wait_current = queue->next;
        struct wait_queue* previous = queue;
        
        insert_wait_queue(wait_current, previous, old_thread);
        
        current_thread = new_thread;
        int tid = new_thread->id_thread;
        
        if(get_and_set_context(old_thread, new_thread)){   
            interrupts_set(enable);
                            //free all memory in my exit queues after a yield occurs
                int enable2 = interrupts_off();
                if(queue_exit->next != NULL){
                    struct queue_list* temp;
                    while(queue_exit->next != NULL){
                        temp = queue_exit->next;

                        if(temp->in_queue->id_thread == 0){
                            //move to next queue if it doesn't hold anything
                            queue_exit->next = temp->next;
                            free(temp);
                        }else{
                            //free all allocated memory
                            free(temp->in_queue->stack);
                            wait_queue_destroy(temp->in_queue->queue_wait);
                            free(temp->in_queue);
                            queue_exit->next = temp->next;
                    
                            free(temp);
                        } 
                    }
                }
                interrupts_set(enable2);
            interrupts_set(enable);
            return tid;
        }        
    }else{
        interrupts_set(enable);
        return THREAD_NONE;
    }
    
    interrupts_set(enable);
    return THREAD_FAILED;
}

/* when the 'all' parameter is 1, wakeup all threads waiting in the queue.
 * returns whether a thread was woken up on not. */
int
thread_wakeup(struct wait_queue *queue, int all)
{
    int enable = interrupts_off();
    
    if(queue == NULL || queue->next == NULL){
        interrupts_set(enable);
        return 0;
    }
/*
    if(queue->next == NULL){
        interrupts_set(enable);
        return 0;
    }
 */
    if(all == 1){
        int i = 0;
        while(queue->next != NULL){
            struct thread* ready_thread = queue->next->wait;

            struct wait_queue* wait_current = queue->next;

            queue->next = queue->next->next; 
            wait_current->wait = NULL;
            free(wait_current);
            
            struct queue_list* current = queue_ready->next;
            struct queue_list* previous = queue_ready;
        
            insert_queue(current, previous, ready_thread);
            
            i++;
        }
    
        interrupts_set(enable);
        return i;
    
    }else{
        
        struct thread* ready_thread = queue->next->wait;

        struct wait_queue* wait_current = queue->next;

        queue->next = queue->next->next; 
        wait_current->wait = NULL;
        free(wait_current);

        struct queue_list* current = queue_ready->next;
        struct queue_list* previous = queue_ready;

        insert_queue(current, previous, ready_thread);

        interrupts_set(enable);
        return 1;
    }
    
    interrupts_set(enable);
    return 0;
}



/* suspend current thread until Thread tid exits */
Tid
thread_wait(Tid tid)
{
    int enable = interrupts_off();
    
    if (tid < 0 || tid == current_thread->id_thread || tid >= THREAD_MAX_THREADS) {
        interrupts_set(enable);
        return THREAD_INVALID;
    }
    
    if(tids[tid] == 0){
        interrupts_set(enable);
        return THREAD_INVALID;   
    }
    
    struct thread* found_thread = NULL;
    struct queue_list* current = queue_ready->next;
    while(current!=NULL && found_thread==NULL) {
        if(current->in_queue->id_thread == tid) found_thread = current->in_queue;
        else found_thread = hellaDeepSearch(current->in_queue->queue_wait,tid);
        
        current = current->next;
    }
    
    thread_sleep(found_thread->queue_wait);
    
    interrupts_set(enable);
    
    return tid;
}

struct lock {
	/* ... Fill this in ... */
    struct thread* thread_lock;
    int captured;
    struct wait_queue* thread_wait;
};

struct lock *
lock_create()
{
    int enable = interrupts_off();
    struct lock *lock;

    lock = malloc(sizeof(struct lock));
    assert(lock);

    lock->captured = 0;
    lock->thread_lock = NULL;
    lock->thread_wait = wait_queue_create();
    
    interrupts_set(enable);
    return lock;
}

void
lock_destroy(struct lock *lock)
{
    int enable = interrupts_off();
    assert(lock != NULL);

    while(1){
        if(lock->captured == 1){
            thread_sleep(lock->thread_wait);
        }else{
            wait_queue_destroy(lock->thread_wait);
            free(lock);
            
            interrupts_set(enable);
            return;
        }
    }

    
}

void
lock_acquire(struct lock *lock)
{
    
    int enable = interrupts_off();
    assert(lock != NULL);

    while(1){
        if(lock->captured == 1){
            thread_sleep(lock->thread_wait);
        }else{
            lock->captured = 1;
            lock->thread_lock = current_thread;
            
            interrupts_set(enable);
            return;
        }
    }

}

void
lock_release(struct lock *lock)
{
    int enable = interrupts_off();
    assert(lock != NULL);

    lock->captured = 0;
    lock->thread_lock = NULL;
    
    thread_wakeup(lock->thread_wait, 1);
    
    interrupts_set(enable);

}

struct cv {
	/* ... Fill this in ... */
    struct wait_queue* cv_wait;
};

struct cv *
cv_create()
{
    interrupts_off();
    struct cv *cv;

    cv = malloc(sizeof(struct cv));
    assert(cv);

    cv->cv_wait = wait_queue_create();

    interrupts_set(interrupts_off());
    return cv;
}

void
cv_destroy(struct cv *cv)
{
    interrupts_off();
    
    assert(cv != NULL);
	
    wait_queue_destroy(cv->cv_wait);
        
    free(cv);
    
    interrupts_set(interrupts_off());
}

void
cv_wait(struct cv *cv, struct lock *lock)
{
    interrupts_off();
    
    assert(cv != NULL);
    assert(lock != NULL);

    if(lock->thread_lock == current_thread){
        lock_release(lock);
        thread_sleep(cv->cv_wait);
        lock_acquire(lock);
    }
    
    interrupts_set(interrupts_off());
}

void
cv_signal(struct cv *cv, struct lock *lock)
{
    interrupts_off();
    
    assert(cv != NULL);
    assert(lock != NULL);

    if(lock->thread_lock == current_thread)
        thread_wakeup(cv->cv_wait, 0);
    
    
    interrupts_set(interrupts_off());
}

void
cv_broadcast(struct cv *cv, struct lock *lock)
{
    interrupts_off();
    
    assert(cv != NULL);
    assert(lock != NULL);

    if(lock->thread_lock == current_thread)
        thread_wakeup(cv->cv_wait, 1);
    
    
    interrupts_set(interrupts_off());
}
