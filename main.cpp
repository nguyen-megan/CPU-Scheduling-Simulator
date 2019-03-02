#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <queue>

using namespace std;

//simple PCB structure
struct PCB{
    int PID;
    int arrival_time;
    int burst_time;
    int response_time;    //1st response time - arrival
    int sim_time;         //time spent in the CPU
    int turnaround_time;  //exit time - arrival time
};

//custom comparator for PCB type based on remaining burst time
struct compare_remaining_burst_time{
    
    //this will allow priority queue to sort in ascending remaining burst time
    bool operator()(PCB const& p1, PCB const& p2){
        return (p1.burst_time-p1.sim_time) > (p2.burst_time-p2.sim_time);
    }
};

bool check_input(int argc, char** argv, string &input_file, string &scheduler_type, int &quantum);
void parse_new_q(vector<string> input_lines, queue<PCB> &new_q);
void FCFS (int clock, queue<PCB>  new_q);
void RR (int clock, int quantum, queue<PCB>  new_q);
void SRTF (int clock, queue<PCB>  new_q);
void calculate_stat(int clock, int idle, int wait, int response, int turnaround, int num);
void update(int clock, queue<PCB> &ready_q, queue<PCB> &new_q);

int main(int argc, char**argv){
    string input_file = "";
    string scheduler_type = "";
    int quantum = 0;
    vector<string> input_lines;  
    ifstream file;
    string line;
    queue<PCB> new_q;
    int clock = 0;

    //make sure all inputs are valid
    if(!check_input(argc, argv, input_file, scheduler_type, quantum)){
        cout << "Invalid Arguments - End Simulation\n";
    }else{
        //try to open file and check if open sucessfully
        file.open(input_file);
        if(!file){ cout << "Failed to open file\n";}
        else{
            //add all lines in file into a list of lines and close file
            while(getline(file, line)){
                input_lines.push_back(line);
            }
            file.close();

            //parse each line by space into PID, arrival, and burst time
            //then add to a queue of new PCB
            parse_new_q(input_lines, new_q);

            cout<<"Scheduling algorithm: " << scheduler_type <<endl;
            cout<<"Total " << new_q.size()<<" tasks are read from "<< input_file << "\n\n";

            //select scheduler
            if(scheduler_type.compare("FCFS") == 0){
                FCFS(clock, new_q);
            }else if(scheduler_type.compare("RR") == 0){
                RR(clock, quantum, new_q);
            }else{
                SRTF(clock, new_q);
            }
        }
    }
    return 0;
}

/*
This function check to make sure that command input has at least 3 arguments (program name, input fle, and scheduler type)
Function also ensure that scheduler type can only be FCFS, RR, or SRTF
If scheduler type is RR, then arguments must include time quantum
*/
bool check_input(int argc, char** argv, string &input_file, string &scheduler_type, int &quantum){
    int result = true;
    if(argc >= 3){ //1st program name, 2nd input_file, 3rd scheduler_type, 4th quantum
        input_file = string(argv[1]);
        scheduler_type = string(argv[2]);
        
        //check for scheduler type.
        if(scheduler_type.compare("FCFS") != 0 &&
         scheduler_type.compare("RR")!= 0 && 
         scheduler_type.compare("SRTF")!= 0){
             cout << "Invalid scheduler\n";
             result = false;
        }else{ //scheduler type is correct
            //if scheduler is RR, then try to obtain quantum
            if(scheduler_type.compare("RR") == 0){
                if(argc > 3){
                    //if time quantum is not a valid integer within acceptable range, output error message
                    quantum = stoi(string(argv[3]));
                }else {
                    cout << "Quantum is required for Round Robin Scheduler\n";
                    result = false;
                }
            }
        }
    }else{
        cout << "Invalid number of arguments \n";
        result = false;
    }
    return result;
}

/*
Function read from a vector of lines (string) and parse each line into 3 elements by space
Elements in each lines correspond to PCB PID, arrival time, and burst time
Add all PCB to a queue of new processes
*/
void parse_new_q(vector<string> input_lines, queue<PCB> &new_q){
    int PID, arrival, burst;
    //read each line and parse
    for(string temp: input_lines){
        istringstream line(temp);
        line >> PID;
        line >> arrival;
        line >> burst;

        //create a new PCB and add to queue
        PCB process{PID, arrival, burst, -1};
        new_q.push(process);
    }
}

/*
This function simulates the First Come First Serve scheduler
Function accepts a clock element and a queue of new process
Function simulates and output message of system clock and current process in CPU
Function output CPU usage, average waiting time, response time, and turnaround time for all processes
*/
void FCFS (int clock, queue<PCB>  new_q){
    int idle = 0;
    queue<PCB> ready_q;
    PCB run;
    int wait=0, response=0, turnaround=0, total_processes = new_q.size();
   
   //keep executing until there's no more elements in new queue
    while(!new_q.empty()){
        
        //if the first process in new queue arrive after current clock time
        //increment clock time, and idle time of CPU, output CPU status
        if(new_q.front().arrival_time > clock){
            clock++;
            idle++;
            printf("<system time %d> no process is running\n", clock);
        }else{
            //update the queues everytime clock ticks
            update(clock, ready_q,new_q);
            
            //keep executing until there's no more elements in ready queue
            while(!ready_q.empty()){
                //assign the first element in ready queue to be run and remove it from ready queue
                run = ready_q.front();
                ready_q.pop();

                //keep running until process simulation time = its burst time  
                //output CPU status, increment process simulation time and clock, update queues
                while(run.sim_time < run.burst_time){
                    printf("<system time %d> process %d is running\n", clock, run.PID);
                    ++run.sim_time;
                    clock++;
                    update(clock, ready_q,new_q);
                }

                //output CPU status about finished process
                printf("<system time %d> process %d is finished...\n", clock, run.PID);
                run.turnaround_time = clock - run.arrival_time;

                //add process time to total time for calculation
                wait += (run.turnaround_time-run.burst_time);
                response += (run.turnaround_time-run.burst_time);
                turnaround += run.turnaround_time;
            }
        }
    }
    //call function to calculate and display scheduler performance
    calculate_stat(clock, idle, wait, response, turnaround,total_processes);
}

/*
This function simulates the Round Robin scheduler
Function accepts a clock element, a time quantum and a queue of new process
Function simulates and output message of system clock and current process in CPU
Function output CPU usage, average waiting time, response time, and turnaround time for all processes
*/
void RR (int clock, int quantum, queue<PCB>  new_q){
    int idle = 0;
    queue<PCB> ready_q;
    PCB run;
    int wait=0, response=0, turnaround=0, total_processes = new_q.size();
    
    //keep executing until there's no more elements in new queue
    while(!new_q.empty()){
        
        //if the first process in new queue arrive after current clock time
        //increment clock time, and idle time of CPU, output CPU status
        if(new_q.front().arrival_time > clock){
            clock++;
            idle++;
            printf("<system time %d> no process is running\n", clock);
        }else{
            //update the queues everytime clock ticks
            update(clock, ready_q,new_q);
            
            //keep executing until there's no more elements in ready queue
            while(!ready_q.empty()){
                //assign the first element in ready queue to be run and remove it from ready queue
                run = ready_q.front();
                
                //if this is the first time process run,
                //assign its response time to be current clock time - arrival time
                if(run.response_time == -1){
                    run.response_time = clock - run.arrival_time;
                }
                ready_q.pop();
                
                //keep running until process simulation time = its burst time or exceed quantum time
                //output CPU status, increment process simulation time and clock, update queues
                for(int i = 0; i < quantum && run.sim_time < run.burst_time;i++){
                    printf("<system time %d> process %d is running\n", clock, run.PID);
                    ++run.sim_time;
                    clock++;
                    update(clock, ready_q,new_q);
                }
                
                //output CPU status about finished process
                //add process time to total time for calculation
                if(run.sim_time == run.burst_time){
                    printf("<system time %d> process %d is finished...\n", clock, run.PID);
                    run.turnaround_time = clock - run.arrival_time;
                    wait += (run.turnaround_time-run.burst_time);
                    response += run.response_time;
                    turnaround += run.turnaround_time;
                }
                
                //if not finish but out of quantum, add process back to ready queue to be run later
                else{
                    ready_q.push(run);
                }
            }
        }
    }
    //call function to calculate and display scheduler performance
    calculate_stat(clock, idle, wait, response, turnaround,total_processes);
}

/*
This function simulates the Shortest Remaining Time First scheduler
Function accepts a clock element, and a queue of new process
Function simulates and output message of system clock and current process in CPU
Function output CPU usage, average waiting time, response time, and turnaround time for all processes
*/
void SRTF (int clock, queue<PCB>  new_q){
    priority_queue<PCB, vector<PCB>, compare_remaining_burst_time> ready_q;
    int idle = 0;
    PCB run;
    int wait=0, response=0, turnaround=0, total_processes = new_q.size();
    
    //keep executing until there's no more elements in new queue
    while(!new_q.empty()){
        
        //if the first process in new queue arrive after current clock time
        //increment clock time, and idle time of CPU, output CPU status
        if(new_q.front().arrival_time > clock){
            printf("<system time %d> no process is running\n", clock);
            clock++;
            idle++;
        }else{
            //update the queues everytime clock ticks
            while(!new_q.empty() && new_q.front().arrival_time <= clock){
                ready_q.push(new_q.front());
                new_q.pop();
            }
           
            //keep executing until there's no more elements in ready queue
            while(!ready_q.empty()){
                //assign the first element in ready queue to be run and remove it from ready queue
                run = ready_q.top();
                
                //if this is the first time process run,
                //assign its response time to be current clock time - arrival time
                if(run.response_time == -1){
                    run.response_time = clock - run.arrival_time;
                }
                ready_q.pop();

                //output CPU status, increment process simulation time and clock, update queues
                printf("<system time %d> process %d is running\n", clock, run.PID);
                ++run.sim_time;
                clock++;
                
                //update the queues everytime clock ticks
                while(!new_q.empty() && new_q.front().arrival_time <= clock){
                    ready_q.push(new_q.front());
                    new_q.pop();
                }

                //output CPU status about finished process
                //add process time to total time for calculation
                if(run.sim_time == run.burst_time){
                    printf("<system time %d> process %d is finished...\n", clock, run.PID);
                    run.turnaround_time = clock - run.arrival_time;
                    wait += (run.turnaround_time-run.burst_time);
                    response += run.response_time;
                    turnaround += run.turnaround_time;
                }
                
                //if not finish, add back to ready queue
                //let process with the next shortest remaining burst time run
                else{
                    ready_q.push(run);  
                }
           }
        }
    }
    //call function to calculate and display scheduler performance
    calculate_stat(clock, idle, wait, response, turnaround,total_processes);
}

/*
This function calculate and display the performance of scheduler
Display percentage of CPU usage, average waiting time, response time, and turnaround time
Accepts system clock, idle time, total wait, resonse, and turnaround time
*/
void calculate_stat(int clock, int idle, int wait, int response,int turnaround, int num){
    printf("<system time %d> All processes finish...\n\n", clock);
    float result;
    
    //CPU usage
    result = 100*(float)(clock-idle)/(float)clock;  //CPU usage
    printf("CPU usage: %.2f %%\n", result);
    
    //Avg wait
    result = (float)(wait)/(float)num;  //wait
    printf("Average waiting time: %.2f\n", result);
    
    //Avg response
    result = (float)(response)/(float)num;  //response
    printf("Average response time: %.2f\n", result);
    
    //Avg turnaround
    result = (float)(turnaround)/(float)num;  //turnaround
    printf("Average turnaround time: %.2f\n\n", result);
}


/*
This function update the new queue and ready queue for FCFS and RR scheduler
everytime clock ticks
*/
void update(int clock, queue<PCB> &ready_q, queue<PCB> &new_q){
    //add all processes in new queue that arrived at or after current clock time to ready queue
    while(!new_q.empty() && new_q.front().arrival_time <= clock){
       ready_q.push(new_q.front());
       new_q.pop();
   }
}