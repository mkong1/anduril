#!/usr/bin/env python

def main(args):
    """Very simple script to guess at driver ADC values for useful voltages,
    based on readings taken at other voltages.
    """
    measurements = {}
    readings_path = args[0]
    for line in open(readings_path):
        if line.startswith('#'):
            continue
        line = line.strip()
        print('Line: %s' % (line))
        parts = line.split()
        if line.endswith('V'):
            blinks = int(parts[0])
            volts = float(parts[2][:-1])
            measurements[volts] = blinks

    v_lowest = min(measurements.keys())
    v_highest = max(measurements.keys())
    b_lowest = measurements[v_lowest]
    b_highest = measurements[v_highest]
    v_range = v_highest - v_lowest
    b_range = b_highest - b_lowest
    b_per_v = b_range / v_range
    v_per_b = v_range / b_range

    #for target_v in [4.20, 4.00, 3.80, 3.50, 3.00, 2.80, 2.70]:
    for target_v in range(44, 19, -1):
        target_v = target_v / 10.0
        volts = target_v - v_lowest
        blinks = b_lowest + (b_per_v * volts)
        #print('%.2f - %.2fV' % (blinks, target_v))
        print('#define ADC_%i     %i' % (target_v * 10, round(blinks)))

if __name__ == "__main__":
    import sys
    main(sys.argv[1:])

