import sys
#import pandas as pd
import os
import numpy as np
import matplotlib.pyplot as plt

file = ""
if len(sys.argv) < 2:
    print("filepath need")
    exit()
filepath = sys.argv[1]
file = open(filepath, 'r')
lines = file.readlines()
file.close()
dict = {}
for line in lines:
    line = line.strip()
    key, vals = line.split(' ')[0], line.split(' ')[1:]
    if dict.__contains__(key):
        dict[key] = dict[key]+1
    else:
        dict[key] = 0
    y = [float(x) for x in vals]
    x = np.linspace(1, len(y), len(y))
    plt.plot(x,y,label=key+'---'+str(dict[key]))
plt.xlabel('generations')
plt.ylabel('fitness')
plt.legend()
plt.show()


