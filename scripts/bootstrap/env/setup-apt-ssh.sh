#!/bin/bash

END=$1
USER_NAME="${USER_NAME}"
REMOTE_HOME="/proj/nova-PG0"
REMOTE_PERSONAL_HOME="$REMOTE_HOME/$USER_NAME"
setup_script="$REMOTE_PERSONAL_HOME/scripts/env"
limit_dir="$REMOTE_PERSONAL_HOME/scripts"
LOCAL_HOME="/Users/baijiadong/Desktop/shaharam-lab/NovaLSM/scripts/bootstrap"
TUTORIAL_HOME="/Users/baijiadong/Desktop/shaharam-lab/NovaLSM/scripts/tutorial"


host="jiadongbai-nova.nova-PG0.apt.emulab.net"

for ((i=0;i<END;i++)); do
    echo "building server on node $i"
    ssh -oStrictHostKeyChecking=no ${USER_NAME}@node-$i.${host} "sudo mkdir ${REMOTE_HOME} ${REMOTE_PERSONAL_HOME} ${limit_dir} ${setup_script}"
    ssh -oStrictHostKeyChecking=no ${USER_NAME}@node-$i.${host} "sudo chmod -R 777 ${REMOTE_HOME}"
    scp -r $LOCAL_HOME/* ${USER_NAME}@node-$i.${host}:$limit_dir/
    scp -r $TUTORIAL_HOME/* ${USER_NAME}@node-$i.${host}:$REMOTE_PERSONAL_HOME/
    if [[ $i == 1 ]]; then
        ssh -oStrictHostKeyChecking=no ${USER_NAME}@node-$i.${host} "cd ${REMOTE_PERSONAL_HOME} && git clone https://github.com/HaoyuHuang/NovaLSM-YCSB-Client && mv NovaLSM-YCSB-Client YCSB-Nova"
    fi
    ssh -oStrictHostKeyChecking=no ${USER_NAME}@node-$i.${host} "cd ${REMOTE_PERSONAL_HOME} && mv *.sh scripts/"
done

for ((i=0;i<END;i++)); do
    echo "building server on node $i"
    ssh -oStrictHostKeyChecking=no ${USER_NAME}@node-$i.${host} "bash $setup_script/setup-ssh.sh"
done

for ((i=0;i<END;i++)); do
    echo "building server on node $i"
    ssh -oStrictHostKeyChecking=no ${USER_NAME}@node-$i.${host} "sudo cp $limit_dir/ulimit.conf /etc/systemd/user.conf"
    ssh -oStrictHostKeyChecking=no ${USER_NAME}@node-$i.${host} "sudo cp $limit_dir/sys_ulimit.conf /etc/systemd/system.conf"
    ssh -oStrictHostKeyChecking=no ${USER_NAME}@node-$i.${host} "sudo cp $limit_dir/limit.conf /etc/security/limits.conf"
    ssh -oStrictHostKeyChecking=no ${USER_NAME}@node-$i.${host} "sudo reboot"
done
