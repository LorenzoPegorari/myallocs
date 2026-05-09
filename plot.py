# MIT License
#
# Copyright (c) 2026 Lorenzo Pegorari (@LorenzoPegorari)
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
## @file plot.py


import matplotlib.pyplot as plt


cycle1 = []
fragmentation = []
cycle2 = []
allocated = []

f1 = open('data1.txt','r')
for row in f1:
    row = row.split(',')
    cycle1.append(int(row[0]))
    fragmentation.append(float(row[1]))
f2 = open('data2.txt','r')
for row in f2:
    row = row.split(',')
    cycle2.append(int(row[0]))
    allocated.append(int(row[1]))

fig, ax1 = plt.subplots()

ax1.set_xlabel('Number of cycles', fontsize=12)
ax1.set_ylabel('Fragmentation percentage', color='r', fontsize=12)
ax1.set_ylim(0, 100)
ax1.plot(cycle1, fragmentation, color='r', label='mymalloc()')
ax1.set_title('Memory fragmentation', fontsize=20)
ax1.grid(color='r', linestyle=':', linewidth=1)
ax1.tick_params(axis='y', labelcolor='r')
ax1.legend(loc='upper left')

ax2 = ax1.twinx()

ax2.set_ylabel('Allocated bytes', color='b', fontsize=12)
ax2.plot(cycle2, allocated, color='b', label='mymalloc()')
ax2.tick_params(axis='y', labelcolor='b')
ax2.legend(loc='upper right')

fig.tight_layout()  # otherwise the right y-label is slightly clipped
plt.show()
