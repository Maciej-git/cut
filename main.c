#include <stdio.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <pthread.h>
#include <stdlib.h>
#include <signal.h>
#include <stdbool.h>

#include <string.h>

#include "functions.c"

// Status signals of threads
bool thr_active[3];


// Contol main loop
bool run = true;

// Contol watchdog status
bool watchdog_active = false;

char *path = "/proc/stat";

// Define Queue for messages
queue log_messages;

pthread_mutex_t readings_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t thr_acive_mutex = PTHREAD_MUTEX_INITIALIZER;


/* Watchdog thread function*/

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
            
        }

        if (thr_active[1] != 1) {
            pthread_mutex_unlock(&thr_acive_mutex);
            printf("Analyzer timedout!\n");
            message w1 = "Thread 'Analyzer' timeout!";
            enqueue(&log_messages, &w1);
            watchdog_active = true;
            run = false;
            
        }

        if (thr_active[2] != 1) {
            pthread_mutex_unlock(&thr_acive_mutex);
            printf("Printer timedout!\n");
            message w2 = "Thread 'Printer' timeout!";
            enqueue(&log_messages, &w2);
            watchdog_active = true;
            run = false;
            
        }
        pthread_mutex_unlock(&thr_acive_mutex);

    }
    
    return 0;
}

/* Reader thread function */ 
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

/* Analyzer thread function */ 
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

/* Printer thread function */
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

    printf("\n");
    for (int i=0; i < get_nprocs(); i++) {
        printf("CPU%i %.2lf %%\n", i, readings[i].cpu_load);
    }

    pthread_mutex_unlock(&readings_mutex);

    return 0;
}

/* Logger thread function*/

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

/* Signal handler SIGINT = break */
void sig_handler(int sig_nr) {
    
    // Break the main loop on interrupt: User break CTRL+C
    run = false;
}

int main (void) {
    pthread_t reader, analyzer, printer, watchdog, logger;

    int nprocs = get_nprocs();

    // Allocate memory according to the processors available in the system.    
    Reading* readings = malloc(nprocs*sizeof(Reading));

    // Check if memory allocation is successful
    if (readings == NULL) {
        message m0 = "Unable to allocate memory!";
        enqueue(&log_messages, &m0);
        logger_thread(&log_messages);
        printf("Unable to allocate memory!\n");
        return 1;

    }

    // Initialise messages queue
    log_messages.front = 0;
    log_messages.size = 0;
   
    printf("This system has %i procesor(s).\n", nprocs);
    printf("\n");

    signal(SIGINT, sig_handler);
    pthread_create(&watchdog, NULL, watchdog_thread, NULL);
   

    while(run) { 
        pthread_create(&logger, NULL, logger_thread, (void *)&log_messages);
        pthread_create(&reader, NULL, reader_thread, (void *)readings);
        pthread_create(&analyzer, NULL, analyzer_thread, (void *)readings);
        pthread_create(&printer, NULL, printer_thread, (void *)readings);
        
        pthread_join(reader, NULL);
        pthread_join(analyzer, NULL);
        pthread_join(printer, NULL);   
    }

    // Free memory and print "good bye" message 
    free(readings);
    
    printf("\n");

    if (!watchdog_active) {
        printf("Program stopped!\n");
    }
    else {

        // Save all queued messages to file
        while (log_messages.size > 0) {
        logger_thread(&log_messages);

    }
        printf("Watchdog stopped the program, please check the log file!\n");
    }
    
    return 0;
}
