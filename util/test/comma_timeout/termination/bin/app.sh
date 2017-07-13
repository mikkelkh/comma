#!/usr/bin/env bash

exec 2>pipe

bash --version >&2

function terminate_call_all()
{
    trap '' INT TERM PIPE HUP
    echo "terminate call_all on signal $1" >&2
    wait $all_pids
    exit 1
}
export -f terminate_call_all

function call_all()
{
    trap 'terminate_call_all TERM' TERM
    trap 'terminate_call_all INT'  INT
    trap 'terminate_call_all HUP'  HUP
    all_pids=""
    ./foo.sh & all_pids+=" $!"
    ./bar.sh & all_pids+=" $!"
    ./baz.sh & all_pids+=" $!"
    wait $all_pids
    echo "call_all terminated" >&2
}
export -f call_all

own_pid=$BASHPID
own_pgid=$( ps -p $own_pid --no-headers -o pgid )
own_pgid=${own_pgid// /}

$input_timeout $input_timeout_options -k 10 -s TERM 20 bash -c call_all & timeout_pid=$!

echo "process group of app: $own_pgid" >&2
echo "process group of timeout: $timeout_pid" >&2

sleep 1
pstree -a -l -c -g -p $BASHPID >&2
echo "app: killing process group $timeout_pid" >&2
kill -TERM -- -$timeout_pid
wait $timeout_pid
echo "app terminated" >&2
