#!/usr/bin/env python

import time
import pylab as pl
import random
import math

def main(args):
    """Simulate thermal regulation
    (because it's easier to test here than in hardware)
    """

    outpath = None
    if args:
        outpath = args[0]

    # general parameter tweaking
    #mAh = 3000.0 * 4  # BLF Q8
    #mAh = 3000.0
    mAh = 700.0
    #mAh = 200.0
    room_temp = 22
    maxtemp = 50
    mintemp = maxtemp - 10
    thermal_mass = 32  # bigger heat sink = higher value
    thermal_lag = [room_temp] * 8
    prediction_strength = 4
    diff_attenuation = 6
    lowpass = 8
    samples = 8
    total_power = 1.0  # max 1.0
    timestep = 0.5  # thermal regulation runs every 0.5 seconds

    # Power level, with max=255
    # 64 steps
    #ramp = [ 1,1,1,1,1,2,2,2,2,3,3,4,5,5,6,7,8,9,10,11,13,14,16,18,20,22,24,26,29,32,34,38,41,44,48,51,55,60,64,68,73,78,84,89,95,101,107,113,120,127,134,142,150,158,166,175,184,193,202,212,222,233,244,255 ]
    # 128 steps
    #ramp = [ 1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,3,3,3,3,3,4,4,4,5,5,5,6,6,7,7,8,8,9,9,10,10,11,12,12,13,14,15,16,17,17,18,19,20,21,22,24,25,26,27,28,30,31,32,34,35,37,39,40,42,44,45,47,49,51,53,55,57,59,61,63,66,68,70,73,75,78,80,83,86,88,91,94,97,100,103,106,109,113,116,119,123,126,130,134,137,141,145,149,153,157,161,166,170,174,179,183,188,193,197,202,207,212,217,222,228,233,238,244,249,255 ]
    # stable temperature at each level, with max=255
    #temp_ramp = [max(room_temp,(lvl/255.0*total_power*200.0)) for lvl in ramp]

    ramp_7135 = [4, 4, 5, 5, 5, 6, 6, 7, 8, 8, 9, 10, 11, 12, 13, 14, 15, 17, 18, 20, 22, 23, 25, 27, 30, 32, 34, 37, 40, 42, 45, 48, 52, 55, 59, 62, 66, 70, 74, 79, 83, 88, 93, 98, 104, 109, 115, 121, 127, 133, 140, 146, 153, 160, 168, 175, 183, 191, 200, 208, 217, 226, 236, 245]
    ramp_FET  = [0, 2, 3, 4, 5, 7, 8, 9, 11, 12, 14, 15, 17, 18, 20, 22, 23, 25, 27, 29, 30, 32, 34, 36, 38, 40, 42, 44, 47, 49, 51, 53, 56, 58, 60, 63, 66, 68, 71, 73, 76, 79, 82, 85, 87, 90, 93, 96, 100, 103, 106, 109, 113, 116, 119, 123, 126, 130, 134, 137, 141, 145, 149, 153, 157, 161, 165, 169, 173, 178, 182, 186, 191, 196, 200, 205, 210, 214, 219, 224, 229, 234, 239, 244, 250, 255]
    tadd_7135 = 5.0
    tadd_FET = 300.0

    ramp = [x/57.0 for x in ramp_7135] + [x for x in ramp_FET]
    temp_ramp = [room_temp + (x/255.0*tadd_7135) for x in ramp_7135]
    temp_ramp.extend([room_temp + tadd_7135 + (x/255.0*tadd_FET) for x in ramp_FET])

    title = 'mAh: %i, Mass: %i, Lag: %i, Ramp: %i, Samples: %i, Predict: %i, Lowpass: %i, Att: %i' % \
            (mAh, thermal_mass, len(thermal_lag), len(ramp), samples,
                    prediction_strength, lowpass, diff_attenuation)

    lvl = int(len(ramp) * 1.0)

    # never regulate lower than this
    lowest_stepdown = len(ramp) / 4

    temperatures = [room_temp] * samples
    actual_lvl = lvl
    overheat_count = 0
    underheat_count = 0
    seconds = 0
    max_seconds = int(mAh * 60.0 / 100.0 * 1.5)

    g_Temit = []
    g_Tdrv = []
    g_act_lvl = []
    g_lvl = []
    g_sag = []
    g_lm = []
    times = []

    def voltage_sag(seconds):
        runtime = float(max_seconds)
        return math.pow((runtime - seconds) / runtime, 1.0/9)

    def get_drv_temp():
        half = int(len(thermal_lag) / 2.0)
        this = sum(thermal_lag[:half]) / float(half)
        val = room_temp + ((this - room_temp) * 0.8)
        #val *= voltage_sag(seconds)
        val = int(val)
        #val += random.choice((-1, 0, 0, 1))
        val += random.choice((-2, -1, 0, 1, 2))
        #val += random.choice((-3, -2, -1, 0, 1, 2, 3))
        del get_drv_temp.values[0]
        get_drv_temp.values.append(val)
        result = sum(get_drv_temp.values) / 1.0
        result = max(room_temp*get_drv_temp.adjust, result)
        return int(result)
    get_drv_temp.values = [room_temp]*4
    get_drv_temp.adjust = 4.0

    def get_temp():
        now = thermal_lag[-1]
        val = room_temp + (voltage_sag(seconds) * (now - room_temp))
        #val *= voltage_sag(seconds)
        result = val
        #result = max(room_temp, result)
        return result

    while True:
        # apply heat step
        target_temp = temp_ramp[actual_lvl-1]
        target_temp = math.pow(target_temp, 1.0/1.01)
        #current_temp = thermal_lag[-1]
        current_temp = get_temp()
        diff = float(target_temp - current_temp)
        current_temp += (diff/thermal_mass)
        current_temp = max(room_temp, current_temp)
        # shift temperatures
        for i in range(len(thermal_lag)-1):
            thermal_lag[i] = thermal_lag[i+1]
        thermal_lag[-1] = current_temp

        # thermal regulation algorithm
        for i in range(len(temperatures)-1):
            temperatures[i] = temperatures[i+1]
        drv_temp = get_drv_temp()
        temperatures[i] = drv_temp
        diff = int(drv_temp - temperatures[0])
        projected = drv_temp + (diff<<prediction_strength)

        #THERM_CEIL = maxtemp<<2
        #THERM_FLOOR = mintemp<<2
        THERM_CEIL = maxtemp * get_drv_temp.adjust
        THERM_FLOOR = mintemp * get_drv_temp.adjust
        if projected > THERM_CEIL:
            underheat_count = 0
            if overheat_count > lowpass:
                overheat_count = 0
                exceed = int(projected-THERM_CEIL) >> diff_attenuation
                exceed = max(1, exceed)
                stepdown = actual_lvl - exceed
                if (stepdown >= lowest_stepdown):
                    actual_lvl = stepdown
            else:
                overheat_count += 1
        elif projected < THERM_FLOOR:
            overheat_count = 0
            if underheat_count > (lowpass/2):
                underheat_count = 0
                if (actual_lvl < lvl):
                    actual_lvl += 1
            else:
                underheat_count += 1

        # show progress
        #print('T=%i: %i/%i, Temit=%i, Tdrv=%i' % (
        #    seconds, actual_lvl, lvl, current_temp, drv_temp))
        g_Temit.append(current_temp)
        g_Tdrv.append(drv_temp/get_drv_temp.adjust)
        g_act_lvl.append(150 * float(actual_lvl) / len(ramp))
        g_lvl.append(150 * float(lvl) / len(ramp))
        g_sag.append(voltage_sag(seconds) * 150.0)
        g_lm.append((voltage_sag(seconds)**2) * ramp[actual_lvl-1] / 255.0 * 150.0)
        times.append(seconds/60.0)

        #time.sleep(0.1)
        seconds += timestep
        if seconds > max_seconds:
            break

    pl.figure(dpi=100, figsize=(12,4))
    pl.suptitle(title, fontsize=14)
    pl.plot(times, g_Temit, label='Temit', linewidth=2)
    pl.plot(times, g_Tdrv, label='Tdrv', linewidth=2)
    pl.plot(times, g_act_lvl, label='actual lvl', linewidth=2)
    pl.plot(times, g_lvl, label='target lvl', linewidth=2)
    pl.plot(times, g_sag, label='voltage sag', linewidth=2)
    pl.plot(times, g_lm, label='lumens', linewidth=2)
    pl.axhspan(mintemp, maxtemp, facecolor='grey', alpha=0.25)
    #pl.axhline(y=mintemp, color='black', linewidth=1)
    pl.xlim((-0.05,times[-1]+0.05))
    pl.xlabel('Minutes')
    pl.ylabel('Temp (C), PWM')
    pl.legend(loc=0)
    if outpath:
        pl.savefig(outpath, dpi=70, frameon=False, bbox_inches='tight')
    pl.show()


if __name__ == "__main__":
    import sys
    main(sys.argv[1:])

