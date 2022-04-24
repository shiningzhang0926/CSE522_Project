sudo mkdir -p /sys/fs/cgroup/resmanager
sudo mkdir -p /sys/fs/cgroup/resmanager/default
sudo chown -R pi:pi /sys/fs/cgroup/resmanager
echo "+memory" > /sys/fs/cgroup/resmanager/cgroup.subtree_control
echo "+cpu" > /sys/fs/cgroup/resmanager/cgroup.subtree_control
