#include "stdio.h"
#include "stdbool.h"
#include "stdlib.h"
#include "stdint.h"
#include "pthread.h"
#include "unistd.h"
#include "string.h"

const int DEFAULT_ARG_THREADS = 1;
const char* USAGE = "lab3 [args]\n\
--threads (-t) [threads]: thread limit (>= 0) (default: 1)";

typedef struct {
    int thread_count;
} t_args;

t_args args;

t_args parse_args(int argc, char** argv) {
    t_args args;
    args.thread_count = DEFAULT_ARG_THREADS;
    bool threads_arg = false;
    int i = 1;
    while (i < argc) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("%s\n", USAGE);
            exit(0);
        }
        if (strcmp(argv[i], "--threads") == 0 || strcmp(argv[i], "-t") == 0) {
            if (threads_arg) {
                printf("overlapping flags\n");
                exit(2);
            }
            threads_arg = true;
            i++;
            if (i == argc) {
                printf("expected threads count\n");
                exit(2);
            }
            int nread = sscanf(argv[i], "%d", &args.thread_count);
            if (nread != 1) {
                printf("failed to read threads count\n");
                exit(2);
            }
            if (args.thread_count <= 0) {
                printf("invalid threads count value\n");
                exit(2);
            }
            i++;
            continue;
        }
        printf("unknown flag %s\n", argv[i]);
        exit(2);
    } 
    return args;
}

pthread_mutex_t thread_mu;
int thread_cnt;

typedef struct {
    void(* func)(void*);
    void* func_args;
} thread_args;

void _thread(void* _th_args) {
    thread_args* th_args = (thread_args*)(_th_args);
    th_args->func(th_args->func_args);

    pthread_mutex_lock(&thread_mu);
    thread_cnt--;
    pthread_mutex_unlock(&thread_mu);
    free(_th_args);
}

void thread(void(* func)(void*), void* func_args) {
    if (func == NULL) {
        return;
    }
    pthread_mutex_lock(&thread_mu);
    if (thread_cnt >= args.thread_count) {
        pthread_mutex_unlock(&thread_mu);
        func(func_args);
        return;
    }
    thread_cnt++;
    pthread_mutex_unlock(&thread_mu);

    thread_args* th_args = malloc(sizeof(thread_args));
    th_args->func = func;
    th_args->func_args = func_args;
    pthread_t thread;
    if (pthread_create(&thread, NULL, (void*)_thread, th_args)) {
        pthread_mutex_lock(&thread_mu);
        thread_cnt--;
        pthread_mutex_unlock(&thread_mu);
        func(func_args);
    }
}

void wait() {
    while (thread_cnt > 1) {
        usleep(1000);
    }
}

typedef struct {
    int* arr;
    int size;
    pthread_mutex_t mutex;
    bool merged;
} run_t;

void insertion_sort(int* arr, int size) {
    for (int i = 1; i < size; i++) {
        int* iter = &arr[i];
        while (iter != arr && *(iter-1) > *iter) {
            int t = *(iter-1);
            *(iter-1) = *iter;
            *iter = t;
            iter--;
        }
    }
}

void reverse(int* arr, int size) {
    for (int i = 0; i <= size/2; i++) {
        int t = arr[i];
        arr[i] = arr[size-i-1];
        arr[size-i-1] = t;
    }
}

typedef struct _list_t {
    run_t run;
    struct _list_t* prev;
}list_t;
list_t* list;
pthread_mutex_t list_mutex;

run_t* list_append(int* arr, int size) {
    list_t* next = malloc(sizeof(list_t));
    next->run.arr = arr;
    next->run.size = size;
    next->run.merged = false;
    pthread_mutex_init(&next->run.mutex, NULL);
    pthread_mutex_lock(&next->run.mutex);
    pthread_mutex_lock(&list_mutex);
    next->prev = list;
    list = next;
    pthread_mutex_unlock(&list_mutex);

    return &next->run;
}

void sort(void* _run) {
    run_t* run = (run_t*)_run;
    if (run->size < 2) {
        pthread_mutex_unlock(&run->mutex);
        return;
    }
    if (run->arr[0] > run->arr[1]) {
        int k = 1;
        for (; k + 1 < run->size && run->arr[k+1] < run->arr[k]; k++);
        reverse(run->arr, k+1);
    }
    insertion_sort(run->arr, run->size);
    pthread_mutex_unlock(&run->mutex);
}

int get_minrun(int n)
{
    int r = 0; 
    while (n >= 64) {
        r |= n & 1;
        n >>= 1;
    }
    return n + r;
}

void mainloop(run_t* array) {
    int minrun = get_minrun(array->size);

    int i = 0;
    while (i < array->size) {
        int j = i;
        if (i + 1 < array->size) j++;
        if (array->arr[i] <= array->arr[j]) {
            for (; j + 1 < array->size && array->arr[j] <= array->arr[j + 1]; j++);
        } else {
            for (; j + 1 < array->size && array->arr[j] > array->arr[j + 1]; j++);
        }
        for (; j + 1 < array->size && j - i + 1 < minrun; j++);

        run_t* run = list_append(&array->arr[i], j - i + 1);
        thread(sort, run);
    
        i = j + 1;
    }
}

void list_dump() {
    printf("list dump:\n");
    for (list_t* ptr = list; ptr != NULL; ptr = ptr->prev) {
        if (!ptr->prev) {
            printf("$\n");
            break;
        }
        printf("%p : [%d]\n", ptr, ptr->run.size);
    }
}

void merge(void* __run2) {
    list_t* _run2 = (list_t*)__run2;
    run_t* run2 = &_run2->run;
    run_t* run1 = &_run2->prev->run;
    int* arr = run1->arr;
    int* temp = malloc(sizeof(int)*run1->size);
    memcpy(temp, arr, sizeof(int)*run1->size);

    int a = 0, b = 0, c = 0;
    while (a < run1->size && b < run2->size) {
        if (temp[a] <= run2->arr[b])
            arr[c++] = temp[a++];
        else arr[c++] = run2->arr[b++];
    }

    for(; a < run1->size; arr[c++] = temp[a++]);
    for(; b < run2->size; arr[c++] = run2->arr[b++]);

    run2->arr = run1->arr;
    run2->size += run1->size;
    run1->merged = true;
    pthread_mutex_unlock(&run1->mutex);
    pthread_mutex_unlock(&run2->mutex);
    free(temp);
}

void mergeloop(void* _array) {
    run_t* array = (run_t*)_array;
    list_t *ptr, *prev;
    do {
        ptr = list;
        prev = list->prev;
        while (ptr && prev && prev->prev) {
            if (pthread_mutex_trylock(&ptr->run.mutex)) {
                ptr = prev;
                prev = ptr->prev;
                continue;
            }
            if (ptr->run.merged) {
                pthread_mutex_unlock(&ptr->run.mutex);
                ptr = prev;
                prev = ptr->prev;
                continue;
            }
            if (pthread_mutex_trylock(&prev->run.mutex)) {
                pthread_mutex_unlock(&ptr->run.mutex);
                ptr = prev;
                prev = ptr->prev;
                continue;
            }
            if (prev->run.merged) {
                pthread_mutex_unlock(&ptr->run.mutex);
                pthread_mutex_lock(&list_mutex);
                ptr->prev = prev->prev;
                pthread_mutex_unlock(&list_mutex);
                pthread_mutex_destroy(&prev->run.mutex);
                free(prev); prev = NULL;
                if (ptr) prev = ptr->prev;
                continue;
            }
            thread(merge, &ptr->run);
            ptr = ptr->prev->prev;
            if (ptr) prev = ptr->prev;
        }
    } while(list->run.size < array->size);
}

void print_run(run_t* run) {
    for (int i = 0; i < run->size; i++) {
        if (i > 0) printf(", ");
        printf("%d", run->arr[i]);
    }
}

int main(int argc, char** argv) {
    srand(time(NULL));
    pthread_mutex_init(&list_mutex, NULL);
    pthread_mutex_init(&thread_mu, NULL);

    thread_cnt = 1;

    args = parse_args(argc, argv);

    int size;
    printf("array size: ");
    scanf("%d", &size);

    run_t array;
    array.arr = malloc(sizeof(int)*size);
    array.size = size;
    for (int i = 0; i < size; i++) {
        array.arr[i] = rand() % 1000;
    }
    printf("array: [%d]int{", size);
    print_run(&array);
    printf("}\n");
    
    list_t _list;
    _list.prev = NULL;
    _list.run.size = 0;
    list = &_list;

    clock_t start = clock();
    if (args.thread_count == 1) {
        mainloop(&array);
        mergeloop(&array);
    } else {
        thread(mergeloop, &array);
        mainloop(&array);
    }

    wait();
    clock_t end = clock();

    printf("\nsorted: [%d]int{", size);
    print_run(&array);
    printf("}\n");

    while (list->prev != NULL) {
        list_t* ptr = list;
        list = list->prev;
        free(ptr);
    }
    free(array.arr);

    pthread_mutex_destroy(&thread_mu);
    pthread_mutex_destroy(&list_mutex);
    printf("time elapsed: %f\n", (double)(end - start) / CLOCKS_PER_SEC);
    return 0;
}