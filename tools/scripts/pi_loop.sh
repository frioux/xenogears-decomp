#!/bin/sh
# Run pi's TUI on PROMPT.md over and over, a fresh session each time.
# Each pass ends when the agent settles (tools/pi/exit-when-settled.ts quits pi).
# To stop: press Escape to abort the run, then quit pi (Ctrl-D or /quit).
# Extra arguments are passed to pi (e.g. --model <pattern>).
cd "$(dirname "$0")/../.." || exit 1

PI_LOOP_MARKER=$(mktemp) || exit 1
export PI_LOOP_MARKER
trap 'rm -f "$PI_LOOP_MARKER"' EXIT

while :; do
    rm -f "$PI_LOOP_MARKER"
    pi -e tools/pi/exit-when-settled.ts "$@" "${PI_LOOP_PROMPT:-read PROMPT.md}"
    [ -e "$PI_LOOP_MARKER" ] || exit
done
