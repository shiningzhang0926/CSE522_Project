CUR_PID=$$
echo $CUR_PID
sudo sh -c "echo $CUR_PID > /sys/fs/cgroup/resmanager/default/cgroup.procs"
