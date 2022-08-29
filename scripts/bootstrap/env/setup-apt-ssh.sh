#!/bin/bash

END=$1
USER_NAME="bobbai"
REMOTE_HOME="/proj/bg-PG0"
REMOTE_PERSONAL_HOME="$REMOTE_HOME/$USER_NAME"
setup_script="$REMOTE_PERSONAL_HOME/scripts/env"
limit_dir="$REMOTE_PERSONAL_HOME/scripts"
bin_dir="$REMOTE_PERSONAL_HOME/nova"
test_case_dir="$REMOTE_PERSONAL_HOME/config"

LOCAL_HOME="/Users/baijiadong/Desktop/shaharam-lab/NovaLSM/scripts/bootstrap"
TUTORIAL_HOME="/Users/baijiadong/Desktop/shaharam-lab/NovaLSM/scripts/tutorial"
BINARY_HOME="/Users/baijiadong/Desktop/shaharam-lab/NovaLSM/dependencies"
TEST_CASE_PATH="/Users/baijiadong/Desktop/shaharam-lab/NovaLSM/config/nova-1-server-1-range-100000000"
LOCAL_YCSB_PATH="/Users/baijiadong/Desktop/shaharam-lab/NovaLSM/scripts/exp/run_ycsb.sh"
PYTHON_PARSER_PATH="/Users/baijiadong/Desktop/shaharam-lab/NovaLSM/scripts/exp/parse_ycsb_nova_leveldb.py"

host="bobbai-nova-5.nova-PG0.apt.emulab.net"

for ((i=0;i<END;i++)); do
    echo "uploading utilities to node $i"
    ssh -oStrictHostKeyChecking=no ${USER_NAME}@node-$i.${host} "sudo mkdir -p ${REMOTE_HOME} ${REMOTE_PERSONAL_HOME} ${limit_dir} ${setup_script} ${test_case_dir} ${bin_dir}"
    ssh -oStrictHostKeyChecking=no ${USER_NAME}@node-$i.${host} "sudo chmod -R 777 ${REMOTE_HOME}"
    scp -r $LOCAL_HOME/* ${USER_NAME}@node-$i.${host}:$limit_dir/
    scp -r $TUTORIAL_HOME/* ${USER_NAME}@node-$i.${host}:$REMOTE_PERSONAL_HOME/
    scp -r $BINARY_HOME/* ${USER_NAME}@node-$i.${host}:$bin_dir/
    scp -r $TEST_CASE_PATH ${USER_NAME}@node-$i.${host}:$test_case_dir/nova-tutorial-config
    scp $LOCAL_YCSB_PATH ${USER_NAME}@node-$i.${host}:$limit_dir/
    scp $PYTHON_PARSER_PATH ${USER_NAME}@node-$i.${host}:$limit_dir/
    ssh -oStrictHostKeyChecking=no ${USER_NAME}@node-$i.${host} "cd ${REMOTE_PERSONAL_HOME} && git clone https://github.com/bobbai00/NovaLSM-YCSB-Client && mv NovaLSM-YCSB-Client YCSB-Nova"
    ssh -oStrictHostKeyChecking=no ${USER_NAME}@node-$i.${host} "cd ${REMOTE_PERSONAL_HOME} && mv *.sh scripts/"
done

for ((i=0;i<END;i++)); do
    echo "setting up ssh on node $i"
    ssh -oStrictHostKeyChecking=no ${USER_NAME}@node-$i.${host} "bash $setup_script/setup-ssh.sh"
done

for ((i=0;i<END;i++)); do
    echo "building server on node $i"
    ssh -oStrictHostKeyChecking=no ${USER_NAME}@node-$i.${host} "sudo cp $limit_dir/ulimit.conf /etc/systemd/user.conf"
    ssh -oStrictHostKeyChecking=no ${USER_NAME}@node-$i.${host} "sudo cp $limit_dir/sys_ulimit.conf /etc/systemd/system.conf"
    ssh -oStrictHostKeyChecking=no ${USER_NAME}@node-$i.${host} "sudo cp $limit_dir/limit.conf /etc/security/limits.conf"
    ssh -oStrictHostKeyChecking=no ${USER_NAME}@node-$i.${host} "sudo reboot"
done
