#!/usr/bin/env bash

output_dir="$1"
iteration=$2

[[ -n "$output_dir" && -n "$iteration" ]] || { echo "$0: empty argument(s)" >&2; exit 1; }

mkdir -p "$output_dir"

function on_exit()
{
    rm -f "$mypipe"
}
trap 'on_exit' EXIT

mypipe="$output_dir/pipe"
rm -f "$mypipe"
mkfifo "$mypipe" || { echo "$0: cannot create named pipe '$mypipe'" >&2; exit 1; }

function handler()
{
    trap '' SIGINT SIGHUP SIGPIPE SIGTERM
    [[ -n "$cat_pid" ]] && {
        kill $cat_pid
        wait $cat_pid
        unset cat_pid
    }
    trap - INT ; kill -INT $BASHPID
}

(
    cd "$output_dir"
    for name in foo.sh bar.sh baz.sh ; do
        rm -f "$name"
        ln -s ../../bin/sub.sh "$name"
    done

    trap 'handler' SIGINT SIGHUP SIGPIPE SIGTERM
    cat > test.log < pipe & cat_pid=$!
    ../../bin/app.sh
    wait $cat_pid ; unset cat_pid

    tail -n 1 test.log | sed "s@^@last[$iteration]=\"@;s@\$@\"@"
)
