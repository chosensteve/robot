n = int(input(''))
l = input('').split(sep=' ')
l = list(map(int, l))
c = 0
if len(l) > 1:
    for i in range(1, n):
        while l[i] < l[i-1]:
            c += l[i-1] - l[i]
            l[i] = l[i-1]    


print(c)