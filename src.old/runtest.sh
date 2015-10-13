#!/bin/bash
bin=$1
score() {
    ./validator.py -i $1 | tail -n -1 | cut -d ' ' -f 2
}
returncode=0
for f in test/*.out ; do
    echo ${f%.out}
    in=${f%.out}.in
    out=$f
    s=$(cat $out | score $in)
    t=$(cat $in | $bin 2>/dev/null | score $in)
    if [ $s -lt $t ] ; then
        echo $in: $t \(\> $s\)
        returncode=$[$returncode + 1]
    fi
done
exit $returncode
