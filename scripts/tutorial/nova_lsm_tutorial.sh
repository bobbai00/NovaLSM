#!/bin/bash
dryrun="false"

recordcount="100000"

bash /proj/bg-PG0/bobbai/scripts/nova_lsm_tutorial_backup.sh $recordcount $dryrun > backup_out
bash /proj/bg-PG0/bobbai/scripts/nova_lsm_tutorial_exp.sh $recordcount $dryrun > exp_out
