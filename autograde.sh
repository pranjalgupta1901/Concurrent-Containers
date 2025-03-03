#!/bin/bash

### Build your executable
make

EXEC="./mysort"
THREADS="4"
chmod u+x $EXEC

TESTDIR=""
if [ $# -eq 1 ]
then
    TESTDIR=$1
else
    TESTDIR="./autograde_tests"
fi

for prim in "treiber" "m_and_s"; do
    for file in $TESTDIR/*; do
        if [ "${file: -4}" == ".cnt" ]; then
            IN=$file
            CASE=${IN%.*}
            read -r ITERS < "$IN"  # Read iterations from the input file
            # Run the mysort program without output file handling
            $EXEC -t "$THREADS" -n "$ITERS" -c $prim
        fi
    done
done

exit $RET
