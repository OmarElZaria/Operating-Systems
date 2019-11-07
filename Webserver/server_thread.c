#include "request.h"
#include "server_thread.h"
#include "common.h"
#include <pthread.h>
#include <semaphore.h>

pthread_t* thread_pool;

struct server {
	int nr_threads;
	int max_requests;
	int max_cache_size;
	int exiting;
        int* r_buffer;
        int outbuffer;
        int inbuffer;
        
        pthread_cond_t full;
        pthread_cond_t empty;
        pthread_mutex_t mutex;
	/* add any other parameters you need */
};



void worker_thread(struct server* sv);

/* static functions */

/* initialize file data */
static struct file_data *
file_data_init(void)
{
	struct file_data *data;

	data = Malloc(sizeof(struct file_data));
	data->file_name = NULL;
	data->file_buf = NULL;
	data->file_size = 0;
	return data;
}


/* free all file data */
static void
file_data_free(struct file_data *data)
{
	free(data->file_name);
	free(data->file_buf);
	free(data);
}

static void
do_server_request(struct server *sv, int connfd)
{
	int ret;
	struct request *rq;
	struct file_data *data;

	data = file_data_init();

	/* fill data->file_name with name of the file being requested */
	rq = request_init(connfd, data);
	if (!rq) {
		file_data_free(data);
		return;
	}
	/* read file, 
	 * fills data->file_buf with the file contents,
	 * data->file_size with file size. */
	ret = request_readfile(rq);
	if (ret == 0) { /* couldn't read file */
		goto out;
	}
	/* send file to client */
	request_sendfile(rq);
out:
	request_destroy(rq);
	file_data_free(data);
}

/* entry point functions */

void worker_thread(struct server* sv){
    
    while(!sv->exiting){
        pthread_mutex_lock(&sv->mutex);
        
        while(sv->outbuffer == sv->inbuffer){
            pthread_cond_wait(&sv->empty, &sv->mutex);
            
            if(sv->exiting){
                pthread_mutex_unlock(&sv->mutex);
                pthread_exit(0);
            }
        }
        
        int connfd = (sv->r_buffer)[sv->outbuffer];
        
        if(((sv->inbuffer - sv->outbuffer + sv->max_requests) % sv->max_requests) == sv->max_requests - 1)
            pthread_cond_broadcast(&sv->full);
        
        sv->outbuffer = (sv->outbuffer + 1) % sv->max_requests;
        
        pthread_mutex_unlock(&sv->mutex);
        
        do_server_request(sv, connfd);
    }
}

struct server *
server_init(int nr_threads, int max_requests, int max_cache_size)
{
	struct server *sv;

	sv = Malloc(sizeof(struct server));
        sv->inbuffer = 0;
        sv->outbuffer = 0;
        pthread_mutex_init(&sv->mutex, NULL);
        pthread_cond_init(&sv->full, NULL);
	pthread_cond_init(&sv->empty, NULL);
	sv->nr_threads = nr_threads;
	sv->max_requests = max_requests + 1;
	sv->max_cache_size = max_cache_size;
	sv->exiting = 0;
        
	/* Lab 4: create queue of max_request size when max_requests > 0 */
	if (nr_threads > 0 || max_requests > 0 || max_cache_size > 0) {

            if(nr_threads <= 0)
                thread_pool = NULL;
            else{
                thread_pool = (pthread_t*)malloc(sizeof(pthread_t)* nr_threads);
                for(unsigned i = 0; i < nr_threads; i++)
                    pthread_create(&thread_pool[i], NULL, (void *)& worker_thread, sv);
            }
            
            if(max_requests <= 0)
                sv->r_buffer = NULL;
            else
                sv->r_buffer = (int*)malloc(sizeof(int)*max_requests);
            
	}
	/* Lab 5: init server cache and limit its size to max_cache_size */
        

	return sv;
}

void
server_request(struct server *sv, int connfd)
{
	if (sv->nr_threads == 0) { /* no worker threads */
            do_server_request(sv, connfd);
	} else {
            /*  Save the relevant info in a buffer and have one of the
                worker threads do the work. */
            pthread_mutex_lock(&sv->mutex);
            
            while(((sv->inbuffer - sv->outbuffer + sv->max_requests) % sv->max_requests) == sv->max_requests -1)
                pthread_cond_wait(&sv->full, &sv->mutex);
            
            (sv->r_buffer)[sv->inbuffer] = connfd;
            
            if(sv->outbuffer == sv->inbuffer)
                pthread_cond_broadcast(&sv->empty);
            
            sv->inbuffer = (sv->inbuffer + 1) % sv->max_requests;
            
            pthread_mutex_unlock(&sv->mutex);
        }
}

void
server_exit(struct server *sv)
{
	/* when using one or more worker threads, use sv->exiting to indicate to
	 * these threads that the server is exiting. make sure to call
	 * pthread_join in this function so that the main server thread waits
	 * for all the worker threads to exit before exiting. */
	sv->exiting = 1;
        
        pthread_cond_broadcast(&sv->full);
        pthread_cond_broadcast(&sv->empty);
        
        for(unsigned i = 0; i < sv->nr_threads; i++){
            pthread_join(thread_pool[i], NULL);
        }
        
        free(sv->r_buffer);
        free(thread_pool);
	/* make sure to free any allocated resources */
	free(sv);
}
