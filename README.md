# ResManager: 
A command-line utility that  will 
1.  Allow the user to dynamically allocate memory and CPU bandwidth to processes.
2.  Monitor and report on memory and CPU bandwidth usage.
3.  Pause the process and warn the user once memory utilization have exceeded their allotted amounts.
4.  Allow the user to decide from allocating additional resources, continue the process without additional resource allocation, or terminate the process.

## About

For our project, we wanted to design a program that would allow us to limit resource utilization of a running process and prevent it from being killed directly by the OOM killer which would result in wasted time and resources from having to rerun the program from the beginning.

## Prerequiste

### Make sure you are using cgroupv2. 
By issuing,
```
mount | grep cgroup
```
make sure that `cgroup2` appears in the output,
```
cgroup2 on /sys/fs/cgroup type cgroup2 (rw,nosuid,nodev,noexec,relatime,nsdelegate)
```
Otherwise, change you cgroup version to v2.

<!-- ### Enable the memory controller in the cgroup.subtree_control
Issue the following command to ensure that the memory controller is enabled in the cgroup controllers.
```
sudo echo "+memory" > cgroup.subtree_control
``` -->


## Install

### make
To install the resmanager and all the test files, run:
```
make clean && make
```

### `cgroup` configuration
**Note that you need SUDO permission for the configuration**

After every system reboot, run ***(this only needs to be run once)**
```
source ./init_cgroup.sh
```
In every newly created terminal, run ***(this needs to be run in every new terminal)**
```
source ./init_resmanager.sh
```


## Usage

### Command-line Options
You may use the resmanager with the following syntax: 

`./resmanager [-mtwb|args] ./user_program [user_program_option]`
* `-m num[KMG]`: Set the maximum amount of memory the user program can use by a number plus unit (K,M,G) like 100K, 10M, etc. The default value is MAX.
* `-t seconds(int)`: Set the time interval of printing current memory usage by a number in seconds. (Integer only, because we used `sleep()`.) The default value is no output.
* `-w weight`: Set the weight of CPU by an integer that ranges from 1 to 10000. The default value is 100.
* `-b bandwidth`: Set the bandwidth of CPU by a decimal that ranges from 0 to 4. The default value is MAX.

### Interactive Commands
1. When the user program is running, the user may pause the execution:
    * `pasue`, `p`: Pause the execution. This makes the `cgroup` in frozen state.

2. When the user program is in frozen state (You may run `pasue` to make user program into frozen state), the user has the following options to allow the ResManager to proceed: 
    * `continue`, `c`: Unfreeze the cgroup and let the user program continue executing.
    * `kill`, `k`: Freeze the cgroup, kill the child proces that executes the user program, remove the cgroup directory and exit.
    * `num[K,M,G]`: Re-allocate larger amount of memory to the user program and let it continue executing.

3. In case the user program needs user input from the terminal:
    * `#`USERINPUT: Use `#` to indicate the ResManager that the characters followed should be forwarded to the user program, including the newline character.

## Examples
### 1. To run a program with memory constraints
To run a simple test that allocate 40KB of memory 50 times with ResManager:
```
./resmanager -m 200KB ./test_increase
```
The expected output in the terminal will be:
```
User allocated max = 204800 bytes from the inputs
Initialize resouce limit successfully
Program starts ... 

vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
Allocate #0 40KB
Allocate #1 40KB
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|Warning: Exceeded memory.high. Proceed with 1 of the 3 options:                  |
|1. Give a new memory.max: num[K,M,G], e.g., 20k, 30M                             |
|2. Proceed:   Type "continue"                                                    |
|3. Terminate: Type "kill"                                                        |
|Please note: Proceeding without adding additional memory is not recommended.     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
Current value of memory.high in bytes: 159744
Current value of memory.max in bytes: 204800
```
The box at the end of the output indicated that the current memory exceeded the memory.high and a new memory allocation is (possibly) needed to allow the test program to finish properly.

### 2. To run a program with cpu constraints
To run a test that creates two dense matrices of size 500 x 500 with randomly-generated values, then multiplies them, using OpenMP to run in parallel. It is limited to 1000 CPU's weight and 0.5 CPU's bandwidth with ResManager:
```
./resmanager -w 1000 -b 0.5 ./parallel_dense_mm 500
```
The expected result can be showed by the shorter program runtime and almost certain CPU usage:
```
[ResManager] Elapsed time of the test program: 11834751 microseconds.
```
The larger the weight of CPU is, the shorter the elaspse time of the user program is. 
```
PID  USER  PR  NI  VIRT  RES  SHR  S  %CPU  %MEM  TIME  COMMAND                                   
2522  pi  20  0  32784  7140  1324  R  49.7  0.8  0:07.88  parallel_dense_         
```
The usage of CPU is 49.7%, which is close to the CPU's bandwidth we set before.

Now the user has three options to continue the execution:
### 3. Allocate more memory
The user may type a memory size in the format of `num[K,M,G]`, e.g., `20K`, `30M`, and press "Enter" in the terminal to allocate more memory for the `memory.max` (`memory.high` will be adjusted accordingly to the predefined ratio to `memory.max`.). The test program will continue to execute with more memory allocated.

Note that the ResManager may freeze the execution again if the memory exceeds
the memory.high again. In this case, the box shown above will show again and ask for another user input.

For example, you may receive the following message when you allocate a larger amount of memory to the test program. The ResManager will report the return status of the test program `status=0`:
```
User input: 30M
New Max: 31457280, Orignal Max: 204800
New memory constraints: Max:31457280, High:25162547
Allocate #2 40KB
Allocate #3 40KB
...
Allocate #48 40KB
Allocate #49 40KB
[test_increase.c] Allocation completed. Begin to free memory.
[test_increase.c] Memory freed. Test Program exits.
[ResManager] Program (./test_increase) exited, status=0
[ResManager] ResManager exits.
```
### 4. Continue the execution
The user may type `continue` to allow the program to continue without additional memory. In this case the freezer will be disabled. The test program may finish properly to be killed by the OOM killer in the cgroup.

For example, you may receive the following message when the OOM killer in invoked:
```
User input: continue
Allocate #2 40KB
Allocate #3 40KB
...
Allocate #10 40KB
Allocate #11 40KB
[ResManager] Program (./test_increase) killed by signal 9
[ResManager] ResManager exits.
```
### 5. Kill the test program
The user may type `kill` to kill the test program directly. ResManager will kill all the child process in the cgroup and clear up the cgroup created.

For example, you may receive the following message when you tyoe `kill` in the terminal:
```
User input: kill
[ResManager] Program (./test_increase) killed by signal 16
[ResManager] ResManager exits.
```

### 6. Request for statistics
We will add another option to allow the user to request the current memory usage and print the statistics in the terminal. 

Let the Resmanager print out the current memory usage of the the running user program every 1 second by setting the option -t.
```
./resmanager -t 1 ./test_increase
```
The expected output in the terminal will be:
```
Allocate #0 40KB
Allocate #1 40KB
Allocate #2 40KB
Allocate #3 40KB
Allocate #4 40KB
Current Memory Usage (bytes): 319488
Allocate #5 40KB
Allocate #6 40KB
Allocate #7 40KB
Allocate #8 40KB
Allocate #9 40KB
Current Memory Usage (bytes): 614400
```
The elapsed time of the user program will be printed out in the end. The expected result looks like:
```
[ResManager] Elapsed time of the test program: 26935 microseconds.
```

## Authors
William Hsaio, Ruiqi Wang, Shining Zhang created for the course project of
CSE 522S
