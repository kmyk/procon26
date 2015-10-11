#!/usr/bin/env python3
import os
import sys
import subprocess
token = open('token').read().strip()
url = 'http://172.16.1.2/answer'
# url = 'http://10.3.17.12/answer'
infile = sys.argv[1]
n = int(open(infile).read().splitlines()[33])
outtxt = []
for line in sys.stdin:
    outtxt.append(line)
    if len(outtxt) == n:
        outtxt = ''.join(outtxt)
        shellcode = 'dos2unix | ./validator.py -i ' + infile + ' | tail -n 1 | cut -d" " -f2'
        proc = subprocess.Popen(shellcode, shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE)
        cout, cerr = proc.communicate(outtxt.encode())
        score = int(cout.strip())
        if not os.path.exists('best'):
            with open('best', 'w') as fh:
                print(1000000007, file=fh)
        best = int(open('best').read().strip())
        if score < best:
            fname = '.submit.{}.txt'.format(os.getpid())
            with open(fname, 'w') as fh:
                print(outtxt, file=fh, end='')
            subprocess.call('curl -F answer=@{} -F token={} {}'.format(fname, token, url), shell=True)
            with open('best', 'w') as fh:
                print(score, file=fh)
        outtxt = []
