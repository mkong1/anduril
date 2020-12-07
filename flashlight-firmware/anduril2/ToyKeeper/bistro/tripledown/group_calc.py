#!/usr/bin/env python

def main(args):
    """group_calc
    This tool calculates evenly-spaced levels for several mode groups
    for the bistro.c firmware.
    """
    lowest = 6
    highest = 40
    for nummodes in range(2, 9):
        stepsize = (highest - lowest) / float(nummodes-1)
        group = []
        level = lowest
        for mode in range(nummodes):
            group.append('%i' % (level))
            level += stepsize
        print(', '.join(group))

if __name__ == "__main__":
    import sys
    main(sys.argv[1:])

