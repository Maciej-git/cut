#include <stdio.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <sys/sysinfo.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define capacity 10

// Structure definition to store CPU data from the /proc/stat file
struct cpustat {
    unsigned long t_user;
    unsigned long t_nice;
    unsigned long t_system;
    unsigned long t_idle;
    unsigned long t_iowait;
    unsigned long t_irq;
    unsigned long t_softirq;
};



// Structre definition to collect CPU data in 2 different time periods
typedef struct Reading {
    double cpu_load;
    struct cpustat st0;
    struct cpustat st1;
} Reading;

typedef char message[255]; 

// Define Queue
typedef struct _queue {
    message *log[capacity];
    atomic_int front;
    atomic_int size;
} queue;

pthread_mutex_t readings_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t thr_acive_mutex = PTHREAD_MUTEX_INITIALIZER;
bool thr_active[3];
bool run = true;
bool watchdog_active = false;
queue log_messages;
char *path = "/proc/stat";

// Add message to queue 
void enqueue(queue* q, message* data) {
    if (q->size < capacity) {
        q->log[(q->front + q->size) % capacity] = data;
        q->size = q->size + 1;      
    }
    
}

// Returns value that was removed from queue
message* dequeue(queue* q) {
    message* data = q->log[q->front];
    q->front = ((q->front + 1) % capacity);
    q->size = q->size - 1;
    
    return data; 

}

// Skip lines in /proc/stat file to reach the selected CPU 
void skip_lines(FILE *fp, int numlines) {
    int cnt = 0;
    char ch;
    while((cnt < numlines) && ((ch = getc(fp)) != EOF))
    {
        if (ch == '\n')
            cnt++;
    }
    return;
}

// Collect data from file and store in cpustat structure
void get_stats(struct cpustat *st, int cpunum, char *path) {
    FILE *fp = fopen(path, "r");

    // Skip the line 0 (overall CPU data)
    int lskip = cpunum + 1;
    skip_lines(fp, lskip);
    char cpun[4];
    fscanf(fp, "%s %ld %ld %ld %ld %ld %ld %ld", cpun, &(st->t_user), &(st->t_nice), 
        &(st->t_system), &(st->t_idle), &(st->t_iowait), &(st->t_irq),
        &(st->t_softirq));
    fclose(fp);
	return;
}

// Calculate load with the given two cpustats 
double calculate_load(struct cpustat *prev, struct cpustat *cur) {
    int idle_prev = (prev->t_idle) + (prev->t_iowait);
    int idle_cur = (cur->t_idle) + (cur->t_iowait);

    int nidle_prev = (prev->t_user) + (prev->t_nice) + (prev->t_system) + (prev->t_irq) + (prev->t_softirq);
    int nidle_cur = (cur->t_user) + (cur->t_nice) + (cur->t_system) + (cur->t_irq) + (cur->t_softirq);

    int total_prev = idle_prev + nidle_prev;
    int total_cur = idle_cur + nidle_cur;

    double totald = (double) total_cur - (double) total_prev;
    double idled = (double) idle_cur - (double) idle_prev;

    double cpu_perc = ((totald - idled) / totald) * 100;

    return cpu_perc;
}

// Watchdog thread function
// Tests thr_active signals
void *watchdog_thread(void *(vptr_value)) {

    while (1) {
        pthread_mutex_lock(&thr_acive_mutex);
        
        // Reset signals from threads
        memset(thr_active, 0, sizeof(thr_active));
    
        pthread_mutex_unlock(&thr_acive_mutex);

        // Wait 2 sec and check signals
        sleep(2);

        pthread_mutex_lock(&thr_acive_mutex);

        // If any thread signal is not set, then stop the program due to timeout.
        if (thr_active[0] != 1) {
            pthread_mutex_unlock(&thr_acive_mutex);
            printf("Reader timedout!\n");
            message w0 = "Thread 'Reader' timeout!";
            enqueue(&log_messages, &w0);
            watchdog_active = true;
            run = false;
            break;    
        }

        if (thr_active[1] != 1) {
            pthread_mutex_unlock(&thr_acive_mutex);
            printf("Analyzer timedout!\n");
            message w1 = "Thread 'Analyzer' timeout!";
            enqueue(&log_messages, &w1);
            watchdog_active = true;
            run = false;
            break;    
        }

        if (thr_active[2] != 1) {
            pthread_mutex_unlock(&thr_acive_mutex);
            printf("Printer timedout!\n");
            message w2 = "Thread 'Printer' timeout!";
            enqueue(&log_messages, &w2);
            watchdog_active = true;
            run = false;
            break;    
        }
        pthread_mutex_unlock(&thr_acive_mutex);
    }
    
    return 0;
}

// Reader thread function
void *reader_thread(void *vptr_value) {
    
    char* bar = "Reader thread active!";
    message* foo;
    foo = (message*)bar;
    
    enqueue(&log_messages, foo);
    

    // Set thread active signal
    pthread_mutex_lock(&thr_acive_mutex);

    thr_active[0] = 1;

    pthread_mutex_unlock(&thr_acive_mutex);

    pthread_mutex_lock(&readings_mutex);

    Reading *readings = (Reading *)vptr_value;
    
    
    // Perform the first CPU data read and the second after a 1 sec delay.
    for (int i=0; i < get_nprocs(); i++) {
        get_stats(&readings[i].st0, i, path);
        
    }
    
    sleep(1);

    for (int i=0; i < get_nprocs(); i++) {

        get_stats(&readings[i].st1, i, path);
        
    }

   pthread_mutex_unlock(&readings_mutex);

   return 0;
}

// Analyzer thread function
void *analyzer_thread(void *vptr_value) {
    
    // Set thread active signal
    pthread_mutex_lock(&thr_acive_mutex);
    thr_active[1] = 1;
    pthread_mutex_unlock(&thr_acive_mutex);

    pthread_mutex_lock(&readings_mutex);

    Reading *readings = (Reading *)vptr_value;

    for (int i=0; i < get_nprocs(); i++) {
            readings[i].cpu_load = calculate_load(&readings[i].st0, &readings[i].st1);
        }

    pthread_mutex_unlock(&readings_mutex);
    
    return 0;

}

// Printer thread function
void *printer_thread(void *vptr_value) {
    
    char* bar = "Printer thread active!";
    message* foo;
    foo = (message*)bar;

    enqueue(&log_messages, foo);
    
    // Set thread active signal
    pthread_mutex_lock(&thr_acive_mutex);

    thr_active[2] = 1;

    pthread_mutex_unlock(&thr_acive_mutex);

    pthread_mutex_lock(&readings_mutex);

    Reading *readings = (Reading *)vptr_value;
    system("clear");
    printf("This system has %i procesor(s).\n\n", get_nprocs());
    for (int i=0; i < get_nprocs(); i++) {
        printf("CPU%i %.2lf %%\n", i, readings[i].cpu_load);
    }

    pthread_mutex_unlock(&readings_mutex);

    printf("\nPress CTRL+C to stop\n");

    return 0;
}

// Logger thread function
void *logger_thread(void *q_data) {
    
    queue *q = (queue *)q_data;
    if (q->size > 0) {
        message *m = dequeue(q);
        FILE *file = fopen("log.txt", "a");
        fprintf(file, "Front: %i Size: %i %s\n", q->front, q->size, *m);
        fclose(file);
    }
    
    return 0;
}

// Signal handler function
void sig_handler(int sig_nr) {
    
    // Break the main loop on interrupt: User break CTRL+C
    run = false;
}
