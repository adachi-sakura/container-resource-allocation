import sys
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

if len(sys.argv) < 2:
    print("filepath need")
    exit()
filepath = sys.argv[1]

df = pd.read_csv(filepath, header=None, sep=" ")
x = np.array(df.iloc[:, 0])
y = np.array(df.iloc[:, 1])

index = np.arange(len(x))
plt.bar(index, y)
for posX, posY in enumerate(y):
    plt.text(posX, posY, '%s'%posY, ha='center', va='bottom')
plt.xlabel('Type')
plt.ylabel('Fitness')
plt.title(filepath)
plt.xticks(index, x)
plt.show()