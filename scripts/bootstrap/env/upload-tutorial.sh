#!/bin/bash

END=$1
USER_NAME=bobbai
host="bobbai-nova-3.nova-PG0.apt.emulab.net"

for ((i=0;i<END;i++)); do
    scp -r /Users/baijiadong/Desktop/shaharam-lab/NovaLSM/scripts/tutorial/* ${USER_NAME}@node-$i.${host}:/proj/bg-PG0/bobbai/scripts/
done