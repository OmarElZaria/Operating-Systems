/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* This is the thread control block */
struct thread {
	 ... Fill this in ... 
    Tid id_thread;
    void* stack;
    struct ucontext context_thread;
};

struct queue_list{
    struct thread* in_queue;
    struct queue_list* next;
};

int tids[MAX_SIZE];
struct queue_list* queue_exit;
struct queue_list* queue_ready;
struct thread* current_thread;

void insert_queue(struct queue_list*  current, struct queue_list*  previous, struct thread*  current_thread);
bool get_and_set_context(struct thread* old_thread,struct thread* new_thread);

/* thread starts by calling thread_stub. The arguments to thread_stub are the
 * thread_main() function, and one argument to the thread_main() function. 
void
thread_stub(void (*thread_main)(void *), void *arg)
{
	thread_main(arg); // call thread_main() function with arg
	thread_exit();
        //free(queue_delete);
        //free(queue_ready);
        exit(0);
}

void
thread_init(void)
{
	/* your optional code here */
    int i;
    
    for(i = 0; i < MAX_SIZE; i++){
        tids[i] = 0;
    }
    tids[0] = 1;
    
    queue_exit = (struct queue_list*)malloc(sizeof(struct queue_list));
    queue_exit->in_queue = NULL;
    queue_exit->next = NULL;
    
    
    queue_ready = (struct queue_list*)malloc(sizeof(struct queue_list));
    queue_ready->in_queue = NULL;
    queue_ready->next = NULL;  
    
    current_thread = (struct thread*)malloc(sizeof(struct thread));
    current_thread->id_thread = 0;
    getcontext(&(current_thread->context_thread));
}

Tid
thread_id()
{
	return current_thread->id_thread;
}

Tid
thread_create(void (*fn) (void *), void *parg)
{
    int new_id = 0;
    
    struct queue_list* current = queue_ready->next;
    struct queue_list* previous = queue_ready;
    
    struct thread* new_thread = (struct thread*)malloc(sizeof(struct thread));
    
//    new_thread->context_thread = (struct ucontext*)malloc(sizeof(struct ucontext));
    
    getcontext(&(new_thread->context_thread));
    
    //if((void*)malloc(THREAD_MIN_STACK) == NULL)
      //  return THREAD_NOMEMORY;
    
    
    for(new_id =0; tids[new_id] == 1; new_id++){
        if((MAX_SIZE-1) == new_id)
            return THREAD_NOMORE;
    }

    void* stack_pointer = malloc(THREAD_MIN_STACK); //+ 24) + THREAD_MIN_STACK +24;
    
    if(stack_pointer == NULL){
        free(new_thread);
        return THREAD_NOMEMORY; 
    }
    
    new_thread->context_thread.uc_mcontext.gregs[REG_RIP] = (unsigned long)&thread_stub;
    new_thread->context_thread.uc_mcontext.gregs[REG_RSP] = ((unsigned long) stack_pointer + THREAD_MIN_STACK) - ((unsigned long) stack_pointer + THREAD_MIN_STACK)%16 - 8;//(unsigned long)stack_pointer; 
    new_thread->context_thread.uc_mcontext.gregs[REG_RSI] = (unsigned long)parg;
    new_thread->context_thread.uc_mcontext.gregs[REG_RDI] = (unsigned long)fn; 
    
    new_thread->context_thread.uc_stack.ss_sp = stack_pointer;
    new_thread->context_thread.uc_stack.ss_size = THREAD_MIN_STACK + 24;
    new_thread->context_thread.uc_stack.ss_flags = 0;
    
    new_thread->stack = stack_pointer;// - (THREAD_MIN_STACK + 24);
    
    new_thread->id_thread = new_id;
    tids[new_id] = 1;
    
    insert_queue(current, previous, new_thread);
    
    return new_thread->id_thread;
}


Tid
thread_yield(Tid want_tid)
{    
    
/*
    if(queue_exit->next != NULL){
        struct queue_list* temp = queue_exit->next;
        while(temp != NULL){
            free(temp->in_queue->context_thread);
            free(temp->in_queue->stack);
            free(temp->in_queue);
            free(temp);
            temp = queue_exit->next;
        }
    }
*/
    if(queue_exit->next != NULL){
        struct queue_list* delete;
        while(queue_exit->next != NULL){
            delete = queue_exit->next;
            if(delete->in_queue->id_thread != 0){
                free(delete->in_queue->stack);
                free(delete->in_queue);
                queue_exit->next = delete->next;
                free(delete);
            }else{
                queue_exit->next = delete->next;
                free(delete);
            } 
        }
    }

    if(current_thread == NULL){
        return THREAD_FAILED;
    }
    if(current_thread->id_thread == want_tid || want_tid == THREAD_SELF){
        volatile int context = 0;
        getcontext(&(current_thread->context_thread));
        if(context){
            return current_thread->id_thread;
        }
        
        context = 1;
        setcontext(&(current_thread->context_thread));
    }else if(want_tid == THREAD_ANY){
        
        if(queue_ready->next != NULL){
            
            struct thread* old_thread;
            old_thread = current_thread;
            
            struct thread* new_thread;
            new_thread = queue_ready->next->in_queue;
            
            struct queue_list* delete = queue_ready->next;
            queue_ready->next = delete->next;
            //queue_ready->next = queue_ready->next->next; 
            
            struct queue_list*  current = queue_ready->next;
            struct queue_list*  previous = queue_ready;;
            
            insert_queue(current, previous, old_thread);
            
            free(delete);
            current_thread = new_thread;
        
            if(get_and_set_context(old_thread, current_thread)){              
                       
                return new_thread->id_thread;
            }
            
        }else{
            return THREAD_NONE;
        }
    
    }else{
        if(want_tid < -2 || want_tid > MAX_SIZE){

            return THREAD_INVALID;
    
        }if(tids[want_tid] == 0 && want_tid >= 0 && want_tid < MAX_SIZE){

            return THREAD_INVALID;
    
        }if(queue_ready->next != NULL){
            struct queue_list* current = queue_ready->next;
            struct queue_list* previous = queue_ready;

            bool key = false;

            while(!key && current!=NULL){
                if(current->in_queue->id_thread != want_tid){
                    previous = current;
                    current = current->next;
                }else{

                    previous->next = current->next;
                    key = true;

                }
            }


            if(!key)
                return THREAD_INVALID;

            struct thread* old_thread;
            old_thread = current_thread;

            struct thread* new_thread;
            new_thread = current->in_queue;
            
            struct queue_list* delete = current;
            current = queue_ready->next;
            previous = queue_ready;

            insert_queue(current, previous, old_thread);
            
            free(delete);
            current_thread = new_thread;

            if(get_and_set_context(old_thread, new_thread))  {          
                
                return new_thread->id_thread;
            }

        }else
            return THREAD_INVALID;    
    }

    return THREAD_FAILED;
}

void
thread_exit()
{
    //TBD();    
    //thread_yield(THREAD_ANY);
    if(current_thread == NULL){
        return;
    }else if(queue_ready->next == NULL){
        //struct thread* thread_exit = current_thread;
        //struct thread* new_thread = queue_ready->next->in_queue;
        //struct queue_list* current = queue_ready->next;   
        
        //tids[current->in_queue->id_thread] = 0;
        //thread_yield(THREAD_ANY);
        
        //struct thread* new_thread;
        //new_thread = queue_ready->next->in_queue;

        //queue_ready->next = queue_ready->next->next;

        //thread_yield(new_thread->id_thread);
        //current_thread = new_thread;
        //setcontext(current_thread->context_thread);
        if(queue_exit->next != NULL){
            struct queue_list* delete;
            while(queue_exit->next != NULL){
                delete = queue_exit->next;
                if(delete->in_queue->id_thread != 0){
                    free(delete->in_queue->stack);
                    free(delete->in_queue);
                    queue_exit->next = delete->next;
                    free(delete);
                }else{
                    queue_exit->next = delete->next;
                    free(delete);
                } 
            }
        }
        exit(0);
        
    }else{
        
        struct queue_list* delete = queue_ready->next;
        queue_ready->next = delete->next;
        
        struct thread* exit_thread = current_thread;
        struct thread* new_thread = delete->in_queue;
        free(delete);
        
        struct queue_list* current = queue_exit->next;
        struct queue_list* previous = queue_exit;
        insert_queue(current, previous, new_thread);       
        
        tids[exit_thread->id_thread] = 0;
        setcontext(&(new_thread->context_thread));
/*
        //if(queue_exit->next != NULL){
          struct queue_list* temp = queue_exit->next;
           while(temp != NULL){
               free(temp->in_queue->context_thread);
               free(temp->in_queue->stack);
               free(temp->in_queue);
               free(temp);
               temp = queue_exit->next;
            }
        }
*/
       // exit(0);
    }
}

Tid
thread_kill(Tid tid)
{
    if(current_thread->id_thread == tid){
        return THREAD_INVALID;
    }
    else if(tid> THREAD_MAX_THREADS || tid <-2)
    {
            return THREAD_INVALID;
    }
    else if(tid< THREAD_MAX_THREADS && tid>=0 && tids[tid]==0)
    {
            return THREAD_INVALID;
    }

    
    else if (queue_ready->next != NULL){
        
        struct queue_list* current = queue_ready->next;
        struct queue_list* previous = queue_ready;
        //struct thread* kill_thread = queue_ready->next->in_queue;
        bool key = false; 


        while(!key && current!=NULL){
            if(current->in_queue->id_thread != tid){
                
                previous = current;
                current = current->next;
            }
            else{
                
                previous->next = current->next;
                key = true;
                
            }
        }

        if(!key)
            return THREAD_INVALID;
        
        struct queue_list* delete = current;
        struct thread* kill_thread = current->in_queue;
        free(delete);
        
        /*
        tids[current->in_queue->id_thread] = 0;
        
        free(current->in_queue->context_thread);
        free(current->in_queue->stack);
        free(current->in_queue);
        free(current);
        
*/
        current = queue_exit->next;
        previous = queue_exit;
        insert_queue(current, previous, kill_thread);       
        
        tids[kill_thread->id_thread] = 0;
        
        return kill_thread->id_thread;    
    }else{
        return THREAD_INVALID;
    }
    
    return THREAD_FAILED;
}

void insert_queue(struct queue_list*  current, struct queue_list*  previous, struct thread* current_thread){
    
    struct queue_list* new_queue = (struct queue_list*) malloc(sizeof(struct queue_list));
    
    while(current!=NULL){
        previous = current;
        current = current->next;
    }
    
    previous->next = new_queue;
    new_queue->next = NULL;
    new_queue->in_queue = current_thread;
    
}

bool get_and_set_context(struct thread* old_thread, struct thread* new_thread){
    volatile bool context = false;
    int err = getcontext(&(old_thread->context_thread));
    assert(!err);
    
    if(context){
                //if(queue_exit->next != NULL){

        /*struct queue_list* temp = queue_exit->next;
        while(temp != NULL){
               free(temp->in_queue->context_thread);
               free(temp->in_queue->stack);
               free(temp->in_queue);
               free(temp);
               temp = queue_exit->next;
         }*/

        
        return true;
    }
    context = true;
    err = setcontext(&(new_thread->context_thread));
    assert(!err);
    
    return false;
}

