d = {}
x = input("numbers: ").split(sep=' ')

for i in x:
    j = int(i)
    d[j] = (j*2, j*3, j*4, j*5)


print(d)