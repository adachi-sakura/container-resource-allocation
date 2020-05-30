import sys
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import os

if len(sys.argv) < 2:
    print("filepath need")
    exit()
filepath = sys.argv[1]

df = pd.read_csv(filepath, header=None, sep=" ")
x = np.array(df.iloc[:, 0])
y = np.array(df.iloc[:, 1])

index = np.arange(len(x))
bar_width=0.3
plt.bar(index, y, bar_width)
for posX, posY in enumerate(y):
    plt.text(posX, posY, '%.3f'%posY, ha='center', va='bottom')
plt.xlabel('Type')
yLabel = os.path.splitext(os.path.basename(filepath))[0]
plt.ylabel(yLabel)
plt.title(filepath)
plt.xticks(index, x)
plt.show()