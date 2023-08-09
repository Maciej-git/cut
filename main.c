#include <stdio.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <pthread.h>
#include <stdlib.h>
#include <signal.h>
#include <stdbool.h>

#include <string.h>

#include "functions.c"

// Contol main loop
extern bool run;

// Contol watchdog status
extern bool watchdog_active;

/* Function prototypes */

// Watchdog thread function
void *watchdog_thread(void *(vptr_value));

// Reader thread function
void *reader_thread(void *vptr_value); 

// Analyzer thread function
void *analyzer_thread(void *vptr_value); 

// Printer thread function
void *printer_thread(void *vptr_value);

// Logger thread function
void *logger_thread(void *q_data);

// Signal handler SIGINT = break
void sig_handler(int sig_nr);


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
