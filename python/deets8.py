d = {}
x = input("numbers: ").split(sep=' ')

for i in x:
    j = int(i)
    k = j*2
    d[(j, k)] = (j+k, j-k, j*k)


print(d)