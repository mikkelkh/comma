#!/usr/bin/env bash

exec 2>pipe

bash --version >&2

function handler()
{
    trap '' TERM INT HUP PIPE
    echo "$0: handle $1, kill $background_pid" >&2
    pstree -a -c -l -g -p $BASHPID >&2
    kill -TERM $background_pid
    wait $background_pid
    sleep 1
    echo "$0: finally exiting" >&2
    exit 1
}

trap 'handler HUP'  HUP
trap 'handler INT'  INT
trap 'handler TERM' TERM

setsid $comma_nap 10 >&2 & background_pid=$!
echo "$0: background process $background_pid" >&2
wait $background_pid

echo "$0 terminated" >&2
