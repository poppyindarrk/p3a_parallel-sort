#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <sys/stat.h>
#include <sys/mman.h>

typedef struct
{
    int key;
    char value[96];
} record;
typedef struct 
{
 int low;
 int high;
 int i;
}TASK;
// get number of threads
int maxthread;
int arrlen;
record **records;

void merge(int low, int mid, int high) {
    // get size of each half
    int n1 = mid - low + 1;
    int n2 = high - mid;
    int i;
    record **left = malloc(n1 * sizeof(record*));
    record **right = malloc(n2 * sizeof(record*));

    for(i = 0; i<n1; i++) {
        left[i] = records[low+i];
        // printf("Left array[%d] set to %d\n", i, left[i]);
    }
    for(int i = 0; i<n2; i++) {
        right[i] = records[mid+i+1];
    }

    // merge
    int l = 0;
    int r = 0;
    int k = low;
    while(l<n1 && r< n2) {
        
        if(left[l]->key < right[r]->key) {
            // printf("HERE HERE HERE\n");
            records[k] = left[l];
            l++;
        } else {
            // printf("HERE HERE HERE HERE\n");
            records[k] = right[r];
            r++;
        }
        k++;
    }


    // add the rest
    while(l<n1) {
        records[k] = left[l];
        l++;
        k++;
    }
    while(r<n2) {
        records[k] = right[r];
        r++;
        k++;
    }
}

void merge_sort(int low, int high) {
    int mid = low + (high - low) / 2;
    if (low < high) {
        merge_sort(low, mid);
        merge_sort(mid + 1, high);
        merge(low, mid, high);
    }
}

void* parallel_sort(void *vargp) {
    
    //pthread_mutex_lock(&mutex1);
     TASK* thread_task = (TASK*) vargp;
     int thread_part = (long)thread_task->i;
    //int thread_part = (long)vargp;
    // calculating low and high
    int low = thread_part * (arrlen / maxthread);
    int high = low + (arrlen / maxthread) - 1;
    if(thread_part == maxthread - 1){
        high = arrlen - 1;
    }
    // evaluating mid point
    int mid = low + (high - low) / 2;
    if (low < high) {
        merge_sort(low, mid);
        merge_sort(mid + 1, high);
        merge(low, mid, high);
    }
    //pthread_mutex_unlock(&mutex1);
    pthread_exit(NULL);
}


int main(int argc, char** argv) {
    char *input_file = argv[1];
    char *output_file = argv[2];   
    struct stat statbuf;
    int len, fd;  
    maxthread = get_nprocs() - 1;
    // get file descriptor and size
    FILE *fp = fopen(input_file, "r"); 
    if (fp == NULL) {
        fprintf(stderr, "An error has occurred\n");
        exit(0);
    }
    fd = fileno(fp);
    fstat(fd, &statbuf);
    len = (int)statbuf.st_size;
    arrlen = len / 100;
    if (len <= 0 || len % 100 != 0) {
        fprintf(stderr, "An error has occurred\n");
        exit(0);
    }

    //converted to char pointer
    //made map private to avoid modifications from other thread.
    char *src = (char *) mmap(0, statbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0 );
    close(fd);
    //create an array of structs
    records = malloc(arrlen * sizeof(record*));

    for (int x = 0; x < arrlen; x++) {
        records[x] = malloc(sizeof(record));
    }
    
    for(int i = 0; i<arrlen; i++) {
        memcpy((void*)records[i], src+100*i, 100);
    }

    pthread_t threads[maxthread];
    TASK* tasklist = (TASK*)malloc(sizeof(TASK) * maxthread);
    TASK* task;
    int low =0;
    int length = arrlen / maxthread;
    for (int i = 0; i < maxthread; i++, low += length)
 {
  task = &tasklist[i];
  task->low = low;
  task->high = low + length - 1;
        task->i = i;
  if (i == (maxthread - 1))
   task->high = arrlen - 1;
 }


    // sort     
    for(long i = 0; i<maxthread; i++) {
        task = &tasklist[i];
        pthread_create(&threads[i], NULL, parallel_sort, (void*)task);
    }
    for(int i = 0; i<maxthread; i++) {
        pthread_join(threads[i], NULL);
    }
    // int size = arrlen/maxthread*2;
    // merge(0, size/2 - 1, size - 1);
    // //printf("sizeis:%d\n",size);
    // //printf("low is:0   mid is:%d   highis:%d\n",size/2 - 1,size - 1);
    // merge(size, size + size / 2-1, size+size-1);
    // //printf("low is:%d   mid is:%d   highis:%d\n",size, size + size / 2-1,size+size-1);
    // merge(0, size-1, size+size-1);
    //printf("low is:0   mid is:%d   highis:%d\n",size - 1,size+size-1);
    //int size = arrlen/maxthread*2;
    TASK* taskm = (TASK*)&tasklist[0];
 for (int i = 1; i < maxthread; i++)
 {
  TASK* task = &tasklist[i];
  merge(taskm->low, task->low-1, task->high);
 }

    FILE *fout = fopen(output_file, "w");
    if (fout == NULL) {
        fprintf(stderr, "An error has occurred\n");
        exit(0);
    }
    for(int i = 0; i< arrlen; i++) {
       fwrite(records[i],sizeof(record),1,fout);
    }
    fclose(fout);
    
    return 0;
}