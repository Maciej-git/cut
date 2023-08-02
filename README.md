# **CUT - CPU Usage Tracker**

### **Description:**

CUT is a console application dedicated to tracking CPU usage based on Linux kernel activity readings from file /proc/stat. The program uses CPU times from /proc/stat to calculate CPU usage for each visible processor. Values are calculated and presented in % with 1-second period as a ratio of CPU usage time to total time since boot. 

### **How to run the application:**

1. In terminal, navigate to project directory.
2. Run 'make' to compile
3. Run './cut' to start the application

Program works in continuous loop, break (CTRL+C) to stop.

### **Project files description:**

### functions.c

The file contains functions needed to open, read /proc/stat file and calculate CPU usage.

### main.c

The file contains application main function and helper functions to run the threads: "Reader", "Analyzer" and "Printer" and SIGTERM handler to catch the break signal (CTRL+C).

### /tests

Directory contains 'testfile' with example CPU data from /proc/stat reading. This file is used to perform unit test, written in 'test_cut.c'. 

To run the unit tests:

1. In terminal, navigate to /tests directory.
2. Run 'make' to compile.
3. Run './test_cut'

If nothing seems to happen, it is a good sign. That means no faults. If any test fails, the corresponding assertion error will be outputted.  