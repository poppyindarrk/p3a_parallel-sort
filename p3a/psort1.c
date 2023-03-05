#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <sys/stat.h>
#include <sys/mman.h>

struct rec {
    int key;
    char value[96];
};
// get number of threads
int num_threads;
int num_records;
struct rec **records;
int curr_partition;

int merge(int left, int mid, int right) {
    // get size of each half
    int size_left = mid - left + 1;
    int size_right = right-mid;
    struct rec **left_array = malloc(size_left * sizeof(struct rec*));
    struct rec **right_array = malloc(size_left * sizeof(struct rec*));

    for(int i = 0; i<size_left; i++) {
        left_array[i] = records[left+i];
        // printf("Left array[%d] set to %d\n", i, left_array[i]);
    }
    for(int i = 0; i<size_right; i++) {
        right_array[i] = records[mid+i+1];
    }

    // merge
    int l = 0;
    int r = 0;
    int i = left;
    while(l<size_left && r< size_right) {
        
        if(left_array[l]->key < right_array[r]->key) {
            // printf("HERE HERE HERE\n");
            records[i] = left_array[l];
            l++;
        } else {
            // printf("HERE HERE HERE HERE\n");
            records[i] = right_array[r];
            r++;
        }
        i++;
    }


    // add the rest
    while(l<size_left) {
        records[i] = left_array[l];
        l++;
        i++;
    }
    while(r<size_right) {
        records[i] = right_array[r];
        r++;
        i++;
    }
    return 0;
}

int merge_sort(int left, int right) {
    if(left<right) {
        int mid = left + (right-left)/2;
        merge_sort(left, mid);
        merge_sort(mid + 1, right);
        merge(left, mid, right);
    }
    return 0;
}

void* merge_caller(void* t) {
    int thread_index = (long) t;
    // get low and high
    int size = num_records/num_threads;
    int low = thread_index * size;
    int high;
    if(thread_index == num_threads-1) {
        high = num_records - 1;
    } else {
        high = low + size-1;
    }
    merge_sort(low, high);
    return 0;
}

int main(int argc, char** argv) {
    FILE *fp;
    int filelen, fd, err;     
    struct stat statbuf;
    num_threads = 1;
    // get file descriptor and size
    fp = fopen(argv[1], "r"); 
    if (fp == NULL) {
        fprintf(stderr, "An error has occurred\n");
        exit(0);
    }
    fd = fileno(fp);
    err = fstat(fd, &statbuf);
    if (err < 0) {
        fprintf(stderr, "An error has occurred\n");
        exit(0);
    }
    filelen = (int)statbuf.st_size;
    num_records = filelen / 100;
    if (filelen <= 0 || filelen % 100 != 0) {
        fprintf(stderr, "An error has occurred\n");
        exit(0);
    }

    //converted to char pointer
    //made map private to avoid modifications from other thread.
    char *ptr = (char *) mmap(0, statbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0 );
    close(fd);
    //create an array of structs
    records = malloc(num_records * sizeof(struct rec*));

    for (int x = 0; x < num_records; x++) {
        records[x] = malloc(sizeof(struct rec));
    }
    
    for(int i = 0; i<filelen; i = i + 100) {
        memcpy((void*)records[i/100], ptr+i, 100);
    }

    pthread_t threads[num_threads];
    int* indexes = malloc(num_threads * sizeof(int));
    // sort     
    for(long i = 0; i<num_threads; i++) {
        indexes[i] = i;
        pthread_create(&threads[i], NULL, merge_caller, (void*)i);
    }
    for(int i = 0; i<num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    int num_sections = num_threads;
    int num_merges = num_sections/2;
    int size = num_records/num_sections*2;
    while(num_merges > 0) {
        int sections = num_sections;
        for(int i = 0; i<num_merges; i++) {
            int low = i * size;
            int high;
            if(num_sections %2 == 0 && i==num_merges-1) {  // i == num_merges-1)
                high = num_records - 1;
            } else {
                high = low + size - 1;
            }
            int mid = low + size/2 - 1;
            merge(low, mid, high);
            sections--;
        }
        size *= 2;
        num_sections = sections;
        num_merges = num_sections/2;
    }
    free(indexes);
    FILE *fdout = fopen(argv[2], "w");
    if (fdout == NULL) {
        fprintf(stderr, "An error has occurred\n");
        exit(0);
    }
    for(int i = 0; i< num_records; i++) {
       fwrite(records[i],sizeof(struct rec),1,fdout);
    }
    fclose(fdout);
    
    return 0;
}