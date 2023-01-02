import math

gamme_table = [int(math.pow(i/255.0, 2.2)*255.0+0.5) for i in range(256)]
for i in range(0, len(gamme_table), 16):
  print ', '.join('%3d'%v for v in gamme_table[i:i+16])

#print len(set(gamme_table))
