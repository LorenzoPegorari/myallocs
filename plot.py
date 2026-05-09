import matplotlib.pyplot as plt

cycle = []
fragmentation = []

f = open('data.txt','r')
for row in f:
    row = row.split(',')
    cycle.append(int(row[0]))
    fragmentation.append(float(row[1]))

plt.plot(cycle, fragmentation, color = 'r', label = 'File Data')

plt.xlabel('Number of cycles', fontsize = 12)
plt.ylabel('Fragmentation percentage', fontsize = 12)

plt.xlim(0)
plt.ylim(0, 100)

plt.grid(color='r', linestyle=':', linewidth=1)

plt.title('Memory fragmentation', fontsize = 20)
plt.legend()
plt.show()
