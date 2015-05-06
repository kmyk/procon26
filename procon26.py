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
