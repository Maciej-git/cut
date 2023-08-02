#include "../functions.c"

#include <stdio.h>
#include <assert.h>
#include <strings.h>
#include <math.h>

// Define a path to test file, containig exemple reading from /proc/stat
char *path = "../tests/testfile";

int main(void) {
    
    FILE *fp = fopen(path, "r");
    char cpu_id[5];

    /* Test 'skip_lines' function */ 
    skip_lines(fp, 1);
    fscanf(fp, "%s ", cpu_id);
    
    // A skip of 1 line should result in "cpu0"
    assert (strcasecmp(cpu_id, "cpu0") == 0);


    /* Test 'get_stats' function */ 
   
    struct cpustat st0;
    // Read data for CPU1 from test file
    get_stats(&st0,1, path);
    assert (st0.t_user == 2344 && st0.t_nice == 0 && st0.t_system == 4886 && st0.t_idle == 641072 
            && st0.t_iowait == 789 && st0.t_irq == 0 && st0.t_softirq == 386);
     

    /* Test 'calculate_load' function */ 

    // Define 2 cpustat structs to simulate varoius readigs 
    struct cpustat st1 = {3192, 0, 8089, 1109510, 1238, 0, 1173};
    struct cpustat st2 = {3198, 0, 8113, 1112322, 1242, 0, 1176};

    // Calculte cpu load with defined structs
    double cpu_load = calculate_load(&st1, &st2);

    // Round calculated value to 2 decimal points
    double rounded = cpu_load * 100;;
    rounded = round(rounded);

    // Expected value is 1.16%
    assert(rounded/100  == 1.16);

    return 0;
}
