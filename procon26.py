# /usr/bin/env python3

import copy

def makelist(*args, default=None):
    if len(args) == 0:
        return default
    else:
        result = []
        for i in range(args[0]):
            result.append(makelist(*args[1:], default=default))
        return result

def on_board(x, y=0):
    return 0 <= x < 32 and 0 <= y < 32
