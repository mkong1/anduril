#!/usr/bin/env python

def main(args):
    cases = []
    for a in range(16):
        for t in range(16):
            cases.append((a,t))

    for a,t in cases:
        diff = t-a
        shift_amount = (diff >> 2) | (diff != 0);
        #shift_amount = ((t - a) >> 2);
        #shift_amount = ((t - a) >> 2) | (t != a);
        #if (t!=a): shift_amount |= 1
        #if (t!=a) and (not shift_amount): shift_amount = 1
        dest = a + shift_amount
        error = ''
        if abs(dest-t) > abs(t-a):
            error = ' ERROR (wrong direction)'
        if (a == t) and (t != dest):
            error = ' ERROR (unnecessary change)'
        if (a != t) and (abs(dest-t) >= abs(t-a)):
            error = ' ERROR (no change)'
        print('(%s -> %s): %s + %s = %s%s' % (a, t, a, shift_amount, dest, error))

if __name__ == "__main__":
    import sys
    main(sys.argv[1:])

