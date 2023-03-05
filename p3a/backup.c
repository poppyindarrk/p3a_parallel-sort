#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <unistd.h>

typedef struct
{
    int key;
    char *value;
} record;

typedef struct 
{
    int low;
    int high;
    int busy;
    record *a;
}TASK;

char error_message[30] = "An error has occurred\n";
int part = 0;
int arrlen = 0;
pthread_mutex_t mutex1;

void merge(record* records, int low, int mid, int high) {
    // n1 is size of left part and n2 is size
    // of right part
    int n1 = mid - low + 1;
    int n2 = high - mid;
    int i, j;
    // creat temp array
    record *left = malloc(n1 * sizeof(record));
    record *right = malloc(n2 * sizeof(record));
    // storing values in left part
    for (i = 0; i < n1; i++) {
        left[i].value = malloc(97);
        memcpy(left[i].value, records[i + low].value, 97);
        left[i].key = records[i + low].key;
    }

    // storing values in right part
    for (i = 0; i < n2; i++) {
        right[i].value = malloc(97);
        memcpy(right[i].value, records[i + mid + 1].value, 97);
        right[i].key = records[i + mid + 1].key;
    }

    int k = low;
    i = j = 0;

    // merge left and right in ascending order
    while (i < n1 && j < n2) {
        if (left[i].key <= right[j].key) {
            records[k++].key = left[i++].key;
            memcpy(records[k++].value, left[i++].value, 97);
        } else {
            records[k++].key = right[j++].key;
            memcpy(records[k++].value, right[i++].value, 97);
        }
    }

    // insert remaining values from left
    while (i < n1) {
        records[k++].key = left[i++].key;
        memcpy(records[k++].value, left[i++].value, 97);
    }

    // insert remaining values from right
    while (j < n2) {
        records[k++] = right[j++];
        memcpy(records[k++].value, right[i++].value, 97);
    }
    i = j = 0;
    while (left[i].value) {
        free(left[i++].value);
    }
    while (right[j].value) {
        free(right[j++].value);
    }
    free(right);
    free(left);
}

void merge_sort(record* a, int low, int high) {
    int mid = low + (high - low) / 2;
    if (low < high) {
        merge_sort(a,low, mid);
        merge_sort(a,mid + 1, high);
        merge(a,low, mid, high);
    }
}

void* parallel_sort(void *vargp) {
    
    TASK *task = (TASK*)vargp;


    // calculating low and high
    int low = task->low;
    int high = task->high;

    // evaluating mid point
    int mid = low + (high - low) / 2;
    if (low < high) {
        merge_sort(task->a,low, mid);
        merge_sort(task->a,mid + 1, high);
        merge(task->a,low, mid, high);
    }
    task->busy = 0;
    return NULL;
}

int main(int argc, char *argv[]) {

    char *input_file = argv[1];
    char *output_file = argv[2];
    struct stat statbuf;
    pthread_mutex_init(&mutex1,NULL);

    if (stat(input_file, &statbuf) == -1) {
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(0);
    }

    if (argv[3] != NULL) {
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(0);
    }
    FILE *fp;
    fp = fopen(input_file, "rb");
    if (fp == NULL) {
        write(STDERR_FILENO, error_message, strlen(error_message));
        return 1;
    }

    fseek(fp, 0L, SEEK_END);
    long int res = ftell(fp); // len of file
    int fdin;
    char *src;
    arrlen = res % 100;
    //printf("arrlen is:%d\n",arrlen);
    fdin = open(argv[1], O_RDONLY);
    src = mmap(0, res, PROT_READ, MAP_SHARED, fdin, 0);
    int i;
    // int index=0;
    record *records = malloc(arrlen * sizeof(record));

    for (i = 0; i < arrlen; i++) { // store key and value from src
        char *str_key = malloc(4*sizeof(char));
        strncpy(str_key, src + 100 * i,4);
        memcpy((void*)&(records[i].key),str_key,4);
        records[i].value = malloc(97);
        memcpy(records[i].value, src + 100 * i + 4, 96);
        strcat(records[i].value, "\0");
    }

    fclose(fp);

    // sort
    int thread_max = 4; // total available threads
    pthread_t *threads = (pthread_t*)malloc(sizeof(pthread_t)*thread_max);
    TASK* tasklist = (TASK*)malloc(sizeof(TASK)*thread_max);

    int len = arrlen/thread_max;

    TASK* task;
    int low = 0;

    for (int i = 0; i < thread_max; i++, low+=len){
        task = &tasklist[i];
        task->low = low;
        task->high = low+len-1;
        if(i == (thread_max - 1)){
            task->high = arrlen - 1;
        }
    }

    
    // creat threads
    for (int i = 0; i < thread_max; i++) {
        task = &tasklist[i];
        task->a = records;
        task->busy = 1;
        pthread_create(&threads[i], 0, parallel_sort, task);
    }
    // join threads 
    // for (int i = 0; i < 4; i++)
    //     pthread_join(threads[i], NULL); 
    for (int i = 0; i < 4; i++)
    {
        while(tasklist[i].busy){
            sleep(50);
        }
    }
    // merge part from threads
    TASK *taskm = &tasklist[0];
    for (int i = 1; i < 4; i++)
    {
        TASK * task = &tasklist[i];
        merge(taskm->a, taskm->low, task->low-1, task->high);
    }

    free(tasklist);
    free(threads);
    FILE *fs;
    fs = fopen(output_file, "w");
    for (int n = 0; n < arrlen; n++) {
        fwrite(records[n].value, sizeof(record), 1, fp);
    }
    fclose(fs);
    return 0;
}