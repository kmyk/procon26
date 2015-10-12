#!/usr/bin/env python3

import sys
import copy
import string
from procon26 import *

def sharpen(x):
    return { '0': ' ', '1': '#' }[x]
def dull(x):
    return { ' ': '0', '#': '1' }[x]

def block_flip(b):
    c = makelist(8,8)
    for y in range(8):
        for x in range(8):
            c[y][8-x-1] = b[y][x]
    return c
def block_rotate_90(b):
    c = makelist(8,8)
    for y in range(8):
        for x in range(8):
            c[x][8-y-1] = b[y][x]
    return c
def block_rotate(b,r):
    assert r == 0 or r == 90 or r == 180 or r == 270
    c = copy.deepcopy(b)
    for i in range(int(r / 90)):
        c = block_rotate_90(c)
    return c

def read_input(f):
    board = [list(map(sharpen,f.readline().strip())) for i in range(32)]
    f.readline()
    n = int(f.readline().strip())
    blocks = []
    for _ in range(n):
        blocks.append([list(map(sharpen,f.readline().strip())) for i in range(8)])
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
    first = True
    for i in range(len(out)):
        if out[i] is not None:
            connected = False
            block = inp['blocks'][i]
            x, y, h, r = out[i]
            if not h:
                block = block_flip(block)
            if r != 0:
                block = block_rotate(block, r)
            for dy in range(8):
                for dx in range(8):
                    if block[dy][dx] == '#':
                        for ddy in [-1,0,1]:
                            for ddx in [-1,0,1]:
                                if on_board(y+dy+ddy, x+dx+ddx):
                                    t = board[y+dy+ddy][x+dx+ddx]
                                    if isinstance(t,int) and t < i:
                                        connected = True
                        if not on_board(y+dy, x+dx): 
                            raise ValueError('{}-th block is not on the board at ({}, {})'.format(i, x+dx, y+dy))
                        if not (board[y+dy][x+dx] == ' '):
                            t = board[y+dy][x+dx]
                            raise ValueError('{}-th block overlaps with {} at ({}, {})'.format(
                                i, ('obstacle' if t == '#' else '{}-th block'.format(t)), x+dx, y+dy))
                        board[y+dy][x+dx] = i
            if not first and not connected:
                raise ValueError('{}-th block is not connected with previous one'.format(i))
            first = False
    return board

def visualize(board, double=False):
    board = copy.deepcopy(board)
    for y in range(32):
        for x in range(32):
            if not isinstance(board[y][x], bool):
                if not double:
                    if isinstance(board[y][x], int):
                        if 0 <= board[y][x] < 10:
                            board[y][x] = str(board[y][x])
                        elif 10 <= board[y][x] < 10+26:
                            board[y][x] = string.ascii_uppercase[board[y][x] - 10]
                        elif 10+26 <= board[y][x] < 10+26+26:
                            board[y][x] = string.ascii_lowercase[board[y][x] - 10-26]
                        else:
                            board[y][x] = '<{}>'.format(int(board[y][x]))
                else:
                    if isinstance(board[y][x], int):
                        board[y][x] = hex(board[y][x])[2:].upper().zfill(2)
                    else:
                        board[y][x] = board[y][x] * 2
    return '\n'.join(map(''.join, board))

def score(board):
    n = 0
    for y in range(32):
        for x in range(32):
            if board[y][x] == ' ':
                n += 1
    return n

def stone(out):
    return len(list(filter(lambda x: x is not None, out)))

if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument('-i', '--input', metavar='PATH')
    parser.add_argument('--score', action='store_true')
    args = parser.parse_args()

    inp = read_input(open(args.input))
    out = read_output(sys.stdin)
    try:
        board = place(inp,out)
    except ValueError as e:
        print('error:', e)
        exit(1)
    else:
        if args.score:
            print('{} {}'.format(score(board), stone(out)))
        else:
            print(visualize(board, double=(10 + 26 + 26 < len(inp['blocks']))))
            print('score: {}'.format(score(board)))
