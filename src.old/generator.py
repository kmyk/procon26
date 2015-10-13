#!/usr/bin/env python3

import sys
import copy
import random
import itertools
from procon26 import *

def read_input(f):
    board = [list(map(sharpen,f.readline().strip())) for i in range(32)]
    f.readline()
    n = int(f.readline().strip())
    blocks = []
    for _ in range(n):
        blocks.append([list(map(sharpen,f.readline().strip())) for i in range(8)])
        f.readline()
    return { 'board': board, 'blocks': blocks }

def generate_random_tiles(h, w, s=None):
    assert s is None or 0 < s
    board = makelist(w, h, default=False)
    while True:
        ly = random.randrange(h)
        ry = random.randrange(ly, (min(ly + s, h) if s is not None else h)) + 1
        lx = random.randrange(w)
        rx = random.randrange(lx, (min(lx + s, w) if s is not None else w)) + 1
        board = copy.deepcopy(board)
        for y in range(ly,ry):
            for x in range(lx,rx):
                board[y][x] = not board[y][x]
        yield board

def flip_tile(xss):
    yss = []
    for xs in xss:
        ys = []
        for x in xs:
            ys.append(not x)
        yss.append(ys)
    return yss

def extend_tile(xss, h, w, default=False):
    yss = makelist(h, w, default=default)
    for y in range(len(xss)):
        for x in range(len(xss[y])):
            yss[y][x] = xss[y][x]
    return yss

def generate_random_board(h, w, s, c):
    assert 0 < h <= 32 and 0 < w <= 32
    assert 1 <= s <= h * w
    if s < h * w / 2:
        is_flip = True
        s = h * w - s
    else:
        is_flip = False
    xss = None
    while not xss:
        it = generate_random_tiles(h, w, max(1, int(min(h, w) * (1 - s / (h * w)))))
        for yss in itertools.islice(it, c, c*2+30):
            if s * 0.9 < sum(yss, []).count(False) < s * 1.1:
                xss = yss
                break
    if is_flip:
        xss = flip_tile(xss)
    xss = extend_tile(xss, 32, 32, True)
    return xss

def is_connected(xss):
    xss = copy.deepcopy(xss)
    h = len(xss)
    w = len(xss[0])
    def f(y, x):
        if 0 <= y < h and 0 <= x < w and xss[y][x]:
            xss[y][x] = False
            g(y, x)
    def g(y, x):
        f(y-1, x)
        f(y+1, x)
        f(y, x-1)
        f(y, x+1)
    n = 0
    for y in range(h):
        for x in range(w):
            if xss[y][x]:
                f(y, x)
                n += 1
                if 2 <= n:
                    return False
    return n == 1

def generate_random_block(h, w, c):
    assert 0 < h <= 8 and 0 < w <= 8
    xss = None
    while not xss:
        for yss in itertools.islice(generate_random_tiles(h,w), c, c+8):
            if not 1 <= sum(yss, []).count(True) <= 16:
                continue
            if not is_connected(yss):
                continue
            if not is_connected(flip_tile(yss)):
                continue
            xss = yss
            break
    xss = extend_tile(xss, 8, 8)
    return xss

def visualize(xss):
    s = []
    for xs in xss:
        for x in xs:
            s.append({ True: '1', False: '0' }[x])
        s.append('\n')
    s.pop()
    return ''.join(s)

if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument('--board-size', type=int, choices=range(1,32+1),
            default=random.randrange(16,32+1))
    parser.add_argument('--board-area', type=float, help='(0,1]',
            default=random.betavariate(4.0,3.0))
    parser.add_argument('--board-complexity', type=int, help='[1,\inf)',
            default=random.randrange(4,32))
    parser.add_argument('--block-number', type=int, choices=range(1,256+1),
            default=random.randrange(1,24))
    parser.add_argument('--block-size', type=int, choices=range(1,8+1),
            default=random.randrange(4,8+1))
    parser.add_argument('--block-complexity', type=int, help='[1,\inf)',
            default=random.randrange(2,5))
    args = parser.parse_args()

    board = generate_random_board(args.board_size, args.board_size,
            args.board_size * args.board_size * args.board_area,
            args.board_complexity)
    print(visualize(board))
    print()
    print(args.block_number)
    blocks = []
    for i in range(args.block_number):
        block = generate_random_block(args.block_size, args.block_size,
                args.block_complexity)
        blocks.append(block)
        if i != 0:
            print()
        print(visualize(blocks[i]))
