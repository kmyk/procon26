#!/bin/bash
bin=$1
score() {
    ./validator.py -i $1 | tail -n -1 | cut -d ' ' -f 2
}
returncode=0
for f in test/*.out ; do
    in=${f%%.out}.in
    out=$f
    s=$(cat $out | score $in)
    t=$(cat $in | $bin 2>/dev/null | score $in)
    if [ $s -lt $t ] ; then
        echo $in: $s \(\< $t of your $out\)
        returncode=$[$returncode + 1]
    fi
done
exit $returncode
