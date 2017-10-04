import sys
import numpy as np
from random import randint

GAP = 1000000000
NUM_SENSOR = 1
NUM_EVENT_PER_SENSOR = 100000000

OUTFILE_PATH = "poisson_timestamps.csv"

f_out = open(OUTFILE_PATH, 'w')

for i in range(0, NUM_SENSOR) :
    data = np.random.poisson(GAP, NUM_EVENT_PER_SENSOR)
    start_point = randint(0, GAP)
    data[0] = start_point
    for j in range(1, NUM_EVENT_PER_SENSOR) :
        data[j] += data[j-1]

    for k in range(0, NUM_EVENT_PER_SENSOR) :
        f_out.write(str(data[k]) + "\n")
        
f_out.close()

