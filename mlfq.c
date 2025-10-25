// Note: Necessary header files are included
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <limits.h>

// Define MAX_NUM_PROCESS
// For simplicity, assume that we have at most 10 processes
#define MAX_NUM_PROCESS 10

#define MAX_PROCESS_NAME 5
#define MAX_GANTT_CHART 300

// N-level Feedback Queue (N=1,2,3,4)
#define MAX_NUM_QUEUE 4

// Keywords (to be used when parsing the input)
#define KEYWORD_QUEUE_NUMBER "queue_num"
#define KEYWORD_TQ "time_quantum"
#define KEYWORD_PROCESS_TABLE_SIZE "process_table_size"
#define KEYWORD_PROCESS_TABLE "process_table"

// Assume that we only need to support 2 types of space characters: 
// " " (space), "\t" (tab)
#define SPACE_CHARS " \t"

// Process data structure
// Helper functions:
//  process_init: initialize a process entry
//  process_table_print: Display the process table
struct Process {
    char name[MAX_PROCESS_NAME];
    int arrival_time ;
    int burst_time;
    int remain_time; // remain_time is needed in the intermediate steps of MLFQ 
    int current_queue; // The queue the process is currently in (0 to queue_num-1)
};
void process_init(struct Process* p, char name[MAX_PROCESS_NAME], int arrival_time, int burst_time) {
    strcpy(p->name, name);
    p->arrival_time = arrival_time;
    p->burst_time = burst_time;
    p->remain_time = burst_time; // Initialize remaining time to burst time
    p->current_queue = 0; // Start in the highest priority queue (Q0)
}
void process_table_print(struct Process* p, int size) {
    int i;
    printf("Process\tArrival\tBurst\n");
    for (i=0; i<size; i++) {
        printf("%s\t%d\t%d\n", p[i].name, p[i].arrival_time, p[i].burst_time);
    }
}


// A simple GanttChart structure
// Helper functions:
//   gantt_chart_print: display the current chart
struct GanttChartItem {
    char name[MAX_PROCESS_NAME];
    int duration;
};

void gantt_chart_print(struct GanttChartItem chart[MAX_GANTT_CHART], int n) {
    int t = 0;
    int i = 0;
    printf("Gantt Chart = ");
    printf("%d ", t);
    for (i=0; i<n; i++) {
        // Skip if duration is 0 (should not happen with proper logic, but as a safeguard)
        if (chart[i].duration <= 0) continue; 
        
        t = t + chart[i].duration;     
        printf("%s %d ", chart[i].name, t);
    }
    printf("\n");
}

// Global variables
int queue_num = 0;
int process_table_size = 0;
struct Process process_table[MAX_NUM_PROCESS];
int time_quantum[MAX_NUM_QUEUE];


// Helper function: Check whether the line is a blank line (for input parsing)
int is_blank(char *line) {
    char *ch = line;
    while ( *ch != '\0' ) {
        if ( !isspace(*ch) )
            return 0;
        ch++;
    }
    return 1;
}
// Helper function: Check whether the input line should be skipped
int is_skip(char *line) {
    if ( is_blank(line) )
        return 1;
    char *ch = line ;
    while ( *ch != '\0' ) {
        if ( !isspace(*ch) && *ch == '#')
            return 1;
        ch++;
    }
    return 0;
}
// Helper: parse_tokens function
void parse_tokens(char **argv, char *line, int *numTokens, char *delimiter) {
    int argc = 0;
    char *token = strtok(line, delimiter);
    while (token != NULL)
    {
        argv[argc++] = token;
        token = strtok(NULL, delimiter);
    }
    *numTokens = argc;
}

// Helper: parse the input file
void parse_input() {
    FILE *fp = stdin;
    char *line = NULL;
    ssize_t nread;
    size_t len = 0;

    char *two_tokens[2]; // buffer for 2 tokens
    char *queue_tokens[MAX_NUM_QUEUE]; // buffer for MAX_NUM_QUEUE tokens
    int n;

    int numTokens = 0, i=0;
    char equal_plus_spaces_delimiters[5] = "";

    char process_name[MAX_PROCESS_NAME];
    int process_arrival_time = 0;
    int process_burst_time = 0;

    strcpy(equal_plus_spaces_delimiters, "=");
    strcat(equal_plus_spaces_delimiters,SPACE_CHARS);    

    // Note: MingGW don't have getline, so you are forced to do the coding in Linux/POSIX supported OS
    // In other words, you cannot easily coding in Windows environment

    while ( (nread = getline(&line, &len, fp)) != -1 ) {
        if ( is_skip(line) == 0)  {
            line = strtok(line,"\n");

            if (strstr(line, KEYWORD_QUEUE_NUMBER)) {
                // parse queue_num
                parse_tokens(two_tokens, line, &numTokens, equal_plus_spaces_delimiters);
                if (numTokens == 2) {
                    sscanf(two_tokens[1], "%d", &queue_num);
                }
            } 
            else if (strstr(line, KEYWORD_TQ)) {
                // parse time_quantum
                parse_tokens(two_tokens, line, &numTokens, "=");
                if (numTokens == 2) {
                    // parse the second part using SPACE_CHARS
                    parse_tokens(queue_tokens, two_tokens[1], &n, SPACE_CHARS);
                    for (i = 0; i < n; i++)
                    {
                        sscanf(queue_tokens[i], "%d", &time_quantum[i]);
                    }
                }
            }
            else if (strstr(line, KEYWORD_PROCESS_TABLE_SIZE)) {
                // parse process_table_size
                parse_tokens(two_tokens, line, &numTokens, equal_plus_spaces_delimiters);
                if (numTokens == 2) {
                    sscanf(two_tokens[1], "%d", &process_table_size);
                }
            } 
            else if (strstr(line, KEYWORD_PROCESS_TABLE)) {

                // parse process_table
                for (i=0; i<process_table_size; i++) {

                    // Loop until a non-skip line is found for the process entry
                    do {
                        getline(&line, &len, fp);
                        // Check for EOF before strtok
                        if (feof(fp) && line[0] == '\0') break; 
                        
                        // Remove newline for parsing
                        char *newline_pos = strchr(line, '\n');
                        if (newline_pos) *newline_pos = '\0';
                        
                    } while (is_skip(line));
                    
                    if (feof(fp) && line[0] == '\0') break; // Break if EOF was reached

                    sscanf(line, "%s %d %d", process_name, &process_arrival_time, &process_burst_time);
                    process_init(&process_table[i], process_name, process_arrival_time, process_burst_time);

                }
            }

        }
        
    }
    if (line) free(line);
}
// Helper: Display the parsed values
void print_parsed_values() {
    printf("%s = %d\n", KEYWORD_QUEUE_NUMBER, queue_num);
    printf("%s = ", KEYWORD_TQ);
    for (int i=0; i<queue_num; i++)
        printf("%d ", time_quantum[i]);
    printf("\n");
    printf("%s = \n", KEYWORD_PROCESS_TABLE);
    process_table_print(process_table, process_table_size);
}

// --- MLFQ Implementation Details ---

// Queue structure: simple array of process indices
// A process is identified by its index in the global process_table
// The maximum size of each queue is MAX_NUM_PROCESS (10), which is safe.
int queues[MAX_NUM_QUEUE][MAX_NUM_PROCESS];
int queue_front[MAX_NUM_QUEUE] = {0};
int queue_rear[MAX_NUM_QUEUE] = {0};

// Helper: Enqueue a process index into a specific queue
void enqueue(int queue_idx, int process_idx) {
    if ((queue_rear[queue_idx] + 1) % MAX_NUM_PROCESS == queue_front[queue_idx]) {
        // Queue is full (should not happen)
        return;
    }
    queues[queue_idx][queue_rear[queue_idx]] = process_idx;
    queue_rear[queue_idx] = (queue_rear[queue_idx] + 1) % MAX_NUM_PROCESS;
    process_table[process_idx].current_queue = queue_idx; // Update process's current queue
}

// Helper: Dequeue a process index from a specific queue
int dequeue(int queue_idx) {
    if (queue_front[queue_idx] == queue_rear[queue_idx]) {
        // Queue is empty
        return -1;
    }
    int process_idx = queues[queue_idx][queue_front[queue_idx]];
    queue_front[queue_idx] = (queue_front[queue_idx] + 1) % MAX_NUM_PROCESS;
    return process_idx;
}

// Helper: Check if a queue is empty
int is_queue_empty(int queue_idx) {
    return queue_front[queue_idx] == queue_rear[queue_idx];
}

// Helper: Find the highest priority non-empty queue
int find_next_queue() {
    for (int i = 0; i < queue_num; i++) {
        if (!is_queue_empty(i)) {
            return i;
        }
    }
    return -1; // All queues are empty
}

// Helper: Add a new item to the Gantt Chart, merging with the previous if possible
void add_gantt_item(struct GanttChartItem chart[MAX_GANTT_CHART], int *sz_chart, char *name, int duration) {
    if (duration <= 0) return;
    
    // Merge with the last item if the process name is the same
    if (*sz_chart > 0 && strcmp(chart[*sz_chart - 1].name, name) == 0) {
        chart[*sz_chart - 1].duration += duration;
    } else {
        // Add a new item
        if (*sz_chart < MAX_GANTT_CHART) {
            strcpy(chart[*sz_chart].name, name);
            chart[*sz_chart].duration = duration;
            (*sz_chart)++;
        }
    }
}

// Helper: Sort process_table by arrival time (needed for correct process arrival)
void sort_process_table() {
    // Simple bubble sort, as MAX_NUM_PROCESS is small (10)
    for (int i = 0; i < process_table_size - 1; i++) {
        for (int j = 0; j < process_table_size - i - 1; j++) {
            if (process_table[j].arrival_time > process_table[j+1].arrival_time) {
                struct Process temp = process_table[j];
                process_table[j] = process_table[j+1];
                process_table[j+1] = temp;
            }
        }
    }
}

// Implementation of MLFQ algorithm
void mlfq() {
    // Sort the process table by arrival time first
    sort_process_table();
    
    struct GanttChartItem chart[MAX_GANTT_CHART];
    int sz_chart = 0;
    long long current_time = 0;
    int processes_done = 0;
    int next_arrival_idx = 0;
    
    // Array to track if a process has been admitted to a queue
    int is_admitted[MAX_NUM_PROCESS] = {0}; 
    
    // Main simulation loop
    while (processes_done < process_table_size && current_time < INT_MAX) {
        
        // 1. Check for new arrivals and add them to the highest priority queue (Q0)
        // This must be done at the start of every time step.
        while (next_arrival_idx < process_table_size && process_table[next_arrival_idx].arrival_time <= current_time) {
            if (!is_admitted[next_arrival_idx]) {
                enqueue(0, next_arrival_idx);
                is_admitted[next_arrival_idx] = 1;
            }
            next_arrival_idx++;
        }
        
        // 2. Find the highest priority non-empty queue
        int current_queue_idx = find_next_queue();
        
        // 3. If all queues are empty, advance time to the next arrival
        if (current_queue_idx == -1) {
            if (next_arrival_idx < process_table_size) {
                // CPU is idle. Advance time to the next process arrival.
                current_time = process_table[next_arrival_idx].arrival_time;
                // Re-check for arrivals at the new time in the next iteration
                continue; 
            } else {
                // All processes are done
                break; 
            }
        }
        
        // 4. Select the process from the front of the selected queue
        int process_idx = dequeue(current_queue_idx);
        struct Process* p = &process_table[process_idx];
        
        // Determine the time slice for this execution
        int time_slice = time_quantum[current_queue_idx];
        
        // Calculate the maximum time we can run before the next event (finish, quantum end, or new arrival)
        int run_time = time_slice;
        if (p->remain_time < run_time) {
            run_time = p->remain_time;
        }
        
        // Find the arrival time of the *next* process that will arrive *after* current_time
        int next_arrival_time = INT_MAX; // Sentinel value
        int i = next_arrival_idx;
        // next_arrival_idx points to the first process that has NOT arrived yet.
        if (i < process_table_size) {
            next_arrival_time = process_table[i].arrival_time;
        }
        
        // If a new process arrives before the current process finishes its calculated run_time,
        // we must preempt and only run until the arrival time.
        // This is the core preemption logic.
        int actual_run_time = run_time;
        
        // Check for preemption by arrival
        if (next_arrival_time < INT_MAX && current_time + actual_run_time > next_arrival_time) {
            // The time to run is limited by the arrival of the next process
            actual_run_time = next_arrival_time - current_time;
        }

        // 5. Execute the process
        if (actual_run_time > 0) {
            p->remain_time -= actual_run_time;
            current_time += actual_run_time;
            
            // Add to Gantt Chart
            add_gantt_item(chart, &sz_chart, p->name, actual_run_time);
        } else {
            // This happens if actual_run_time is 0, meaning current_time == next_arrival_time, and the process 
            // was selected, but the next arrival should preempt it. Since we dequeued it, we must re-enqueue it.
            enqueue(p->current_queue, process_idx);
            continue; // Skip the rest of the loop and re-evaluate
        }
        
        // 6. Post-execution handling
        if (p->remain_time > 0) {
            // Process is not finished.
            
            // Check if preemption by arrival occurred (current_time == next_arrival_time)
            if (current_time == next_arrival_time) {
                // Preempted by arrival. Re-enqueue to the same queue.
                enqueue(p->current_queue, process_idx);
            } 
            // Check if the process ran for its full quantum.
            else if (actual_run_time == time_slice) {
                // Ran until quantum end. Demote (or re-enqueue in the last queue).
                
                int next_queue_idx = p->current_queue + 1;
                
                if (next_queue_idx < queue_num) {
                    // Demote
                    enqueue(next_queue_idx, process_idx);
                } else {
                    // Already in the lowest priority queue (FCFS - large TQ)
                    // Re-enqueue to the same queue
                    enqueue(p->current_queue, process_idx);
                }
            } else {
                // Ran less than quantum, did not finish, and was not preempted by arrival.
                // This means the process was limited by its remaining time, which is only possible if 
                // run_time was limited by remain_time, but the remaining time is still > 0.
                // This case should not happen if the logic is correct, but for safety, re-enqueue.
                // The only case where actual_run_time < time_slice and p->remain_time > 0 is if 
                // actual_run_time was limited by p->remain_time, which means p->remain_time should be 0.
                // The only other case is if run_time was limited by p->remain_time, and p->remain_time is now 0.
                // The logic here is flawed. If p->remain_time > 0, it means it was preempted by arrival.
                // If it wasn't preempted by arrival (current_time != next_arrival_time) and didn't use full slice,
                // it must have finished, which contradicts p->remain_time > 0.
                // The only remaining case is if it was limited by run_time, but the remaining time is still > 0.
                // This is a bug in the logic. Let's re-examine the logic.
                // The correct logic is: if p->remain_time > 0, re-enqueue to the same queue.
                // The "else if (actual_run_time == time_slice)" handles the demotion.
                // The "else" block is for when actual_run_time < time_slice and current_time != next_arrival_time.
                // This can only happen if run_time was limited by p->remain_time, which means p->remain_time should be 0.
                // Since p->remain_time > 0, it must be the arrival preemption case.
                // Let's remove the "else" block and rely on the two checks.
                
                // Re-enqueue to the same queue.
                enqueue(p->current_queue, process_idx);
            }
            
        } else {
            // Process is finished.
            processes_done++;
        }
        
        // Safety break for MAX_GANTT_CHART
        if (current_time >= MAX_GANTT_CHART && processes_done < process_table_size) {
            // If the chart is full but not all processes are done, we stop the chart printing
            // but the simulation should continue until all processes are done.
            // However, for this assignment, we only care about the Gantt chart.
            break;
        }
    }
    
    // At the end, display the final Gantt chart
    gantt_chart_print(chart, sz_chart);

}


int main() {
    parse_input();
    print_parsed_values();
    mlfq();
    return 0;
}
