#include <stdio.h>
#include <pthread.h>

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

