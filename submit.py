#!/usr/bin/env python3
import os
import sys
import subprocess
token = open('token').read().strip()
url = 'http://172.16.1.2/answer'
# url = 'http://10.3.17.12/answer' # rkx's one
infile = sys.argv[1]
n = int(open(infile).read().splitlines()[33])
outtxt = []
for line in sys.stdin:
    outtxt.append(line)
    if len(outtxt) == n:
        outtxt = ''.join(outtxt)
        shellcode = 'dos2unix | ./validator.py --score -i ' + infile
        proc = subprocess.Popen(shellcode, shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE)
        cout, cerr = proc.communicate(outtxt.encode())
        score = list(map(int,cout.strip().split()))
        if not os.path.exists('best'):
            with open('best', 'w') as fh:
                print(1000000007, 1000000007, file=fh)
        best = list(map(int,open('best').read().strip().split()))
        if score < best:
            fname = '.submit.{}.txt'.format(os.getpid())
            with open(fname, 'w') as fh:
                print(outtxt, file=fh, end='')
            print('submit: score {}, stone {}'.format(*best))
            subprocess.call('curl -F answer=@{} -F token={} {}'.format(fname, token, url), shell=True)
            with open('best', 'w') as fh:
                print(*score, file=fh)
        outtxt = []
