#include <stdio.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <pthread.h>
#include <stdlib.h>
#include <signal.h>
#include <stdbool.h>

#include "functions.c"

// Contol main loop
bool run = true;

char *path = "/proc/stat";

pthread_mutex_t readings_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Reader thread function */ 
void *reader_thread(void *vptr_value) {

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

    pthread_mutex_lock(&readings_mutex);

    Reading *readings = (Reading *)vptr_value;

    printf("\n");
    for (int i=0; i < get_nprocs(); i++) {
        printf("CPU%i %.2lf %%\n", i, readings[i].cpu_load);
    }

    pthread_mutex_unlock(&readings_mutex);

    return 0;
}

/* Signal handler SIGINT = break */
void sig_handler(int sig_nr) {
    printf("\n");
    printf("Program stopped with %d (SIGINT), last reading:\n", sig_nr);

    // Break the main loop on interrupt: User break CTRL+C
    run = false;
}

int main (void) {
    pthread_t reader, analyzer, printer;

    int nprocs = get_nprocs();

    // Allocate memory according to the processors available in the system.    
    Reading* readings = malloc(nprocs*sizeof(Reading));
    
    // Check if memory allocation is successful
    if (readings == NULL) {

        printf("Unable to allocate memory!");
        return 1;

    }

    printf("This system has %i procesor(s).\n", nprocs);
    printf("\n");

    signal(SIGINT, sig_handler);

    while(run) {
        pthread_create(&reader, NULL, reader_thread, (void *)readings);
        pthread_create(&analyzer, NULL, analyzer_thread, (void *)readings);
        pthread_create(&printer, NULL, printer_thread, (void *)readings);
        pthread_join(reader, NULL);
        pthread_join(analyzer, NULL);
        pthread_join(printer, NULL);

    }
    
    // Free memory
    free(readings);
    
    return 0;
}
