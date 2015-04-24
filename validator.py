#!/usr/bin/env python3

import sys
import copy

def from_zero_one_string(s):
    return list(map(lambda x: x == '1', s))
def to_zero_one_string(s):
    return ''.join(map(lambda x: '1' if x else '0', s))

def makelist(*args, default=None):
    if len(args) == 0:
        return None
    else:
        result = []
        for i in range(args[0]):
            result.append(makelist(*args[1:], default=default))
        return result

def block_flip(b):
    c = makelist(8,8)
    for y in range(8):
        for x in range(8):
            c[y][8-x-1] = b[y][x]
    return c
def block_rotate(b,r):
    assert r == 0 or r == 90 or r == 180 or r == 270
    c = makelist(8,8)
    for y in range(8):
        for x in range(8):
            if r == 90:
                c[x][8-y-1] = b[y][x]
            elif r == 180:
                c[8-y-1][8-x-1] = b[y][x]
            elif r == 270:
                c[8-x-1][y] = b[y][x]
            else:
                c[y][x] = b[y][x]
    return c
def block_visualize(b):
    return '\n'.join(map(to_zero_one_string, b))

def read_input(f):
    board = [from_zero_one_string(f.readline().strip()) for i in range(32)]
    f.readline()
    n = int(f.readline().strip())
    blocks = []
    for _ in range(n):
        blocks.append([from_zero_one_string(f.readline().strip()) for i in range(8)])
        f.readline()
    return { 'board': board, 'blocks': blocks }

def generate_output(f):
    for line in f:
        line = line.strip()
        if not line:
            yield None
        else:
            x, y, h, r = line.split()
            x = int(x)
            y = int(y)
            h = h == 'H'
            r = int(r)
            yield x, y, h, r
def read_output(f):
    return list(generate_output(f))

def place(inp,out):
    assert len(inp['blocks']) == len(out)
    board = copy.deepcopy(inp['board'])
    for i in range(len(out)):
        if out[i] is not None:
            block = inp['blocks'][i]
            x, y, h, r = out[i]
            if not h:
                block = block_flip(block)
            if r != 0:
                block = block_rotate(block, r)
            for dy in range(8):
                for dx in range(8):
                    if block[dy][dx]:
                        assert 0 <= y+dy < 32
                        assert 0 <= x+dx < 32
                        assert board[y+dy][x+dx] == False
                        board[y+dy][x+dx] = i
    return board

def visualize(*args):
    board = place(*args)
    for y in range(32):
        for x in range(32):
            if isinstance(board[y][x],bool):
                board[y][x] = '#' if board[y][x] else ' '
            else:
                board[y][x] = str(board[y][x])
    return '\n'.join(map(''.join, board))

def score(*args):
    board = place(*args)
    n = 0
    for y in range(32):
        for x in range(32):
            if isinstance(board[y][x],bool) and board[y][x] == False:
                n += 1
    return n

if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument('-i', '--input', metavar='PATH')
    args = parser.parse_args()

    inp = read_input(open(args.input))
    out = read_output(sys.stdin)
    block = inp['blocks'][0]
    print(visualize(inp,out))
    print('score: {}'.format(score(inp,out)))
