# ResManager: 
A command-line utility that  will 
1.  Allow the user to dynamically allocate memory and CPU bandwidth to processes.
2.  Monitor and report on memory and CPU bandwidth usage.
3.  Pause the process and warn the user once memory utilization have exceeded their allotted amounts.
4.  Allow the user to decide between allocating additional resources, continue the process without additional resource allocation, or terminate the process.

## About

For our project, we wanted to design a program that would allow us to limit resource utilization of a running process and prevent it from being killed directly by the OOM killer which would result in wasted time and resources from having to rerun the program from the beginning.

## Install
To install the resmanager and all the test files, run:
```
make clean && make
```


## Run

To run a
```
sudo ./resmanager -m 200 -u KB ./test_increase
```

