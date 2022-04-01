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
You should find something like,
```
cgroup2 on /sys/fs/cgroup type cgroup2 (rw,nosuid,nodev,noexec,relatime,nsdelegate)
```
Otherwise, change you cgroup version to v2.

### Enable the memory controller in the cgroup.subtree_control
Issue
```
sudo echo "+memory" > cgroup.subtree_control
```
## Install
**Note that you need SUDO permission to run the ResManager**

To install the resmanager and all the test files, run:
```
make clean && make
```


## Usage and Tests

**Note that you need SUDO permission to run the ResManager**

To run a simple test that allocate 40KB of memory 50 times with ResManager:
```
sudo ./resmanager -m 200 -u KB ./test_increase
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
|1. Give a new memory.max: num[k,M,G], e.g., 20k, 30M                             |
|2. Proceed:   Type "continue"                                                    |
|3. Terminate: Type "kill"                                                        |
|Please note: Proceeding without adding additional memory is not recommended.     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```
The box at the end of the output indicated that the current memory exceeded the memory.high and a new memory allocation is (possibly) needed to allow the test program to finish properly.

Now the user has three options to continue the execution:
### 1. Allocate more memory
The user may type a memory size in the format of `num[k,M,G]`, e.g., `20k`, `30M`, and press "Enter" in the terminal to allocate more memory for the `memory.max` (`memory.high` will be adjusted accordingly to the predefined ratio to `memory.max`.). The test program will continue to execute with more memory allocated.

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
### 2. Continue the execution
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
### 3. Kill the test program
The user may type `kill` to kill the test program directly. ResManager will kill all the child process in the cgroup and clear up the cgroup created.

For example, you may receive the following message when you tyoe `kill` in the terminal:
```
User input: kill
[ResManager] Program (./test_increase) killed by signal 16
[ResManager] ResManager exits.
```

### (TODO) 4. Request for statistics
We will add another option to allow the user to request the current memory usage and print the statistics in the terminal. 

## Authors
William Hsaio, Ruiqi Wang, Shining Zhang created for the course project of
CSE 522S
