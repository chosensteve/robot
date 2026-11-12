x = int(input("x: "))
n = int(input("n: "))

sum = [0,0,0,0]

for i in range(n+1):
    print(x**i, end=' ')
    sum[0] += x**i

print('\n', sum[0])

for i in range(n+1):
    print((-x)**i, end=' ')
    sum[1] += (-x)**i

print('\n', sum[1])

for i in range(1, n):
    print((x**n)/n, end=' ')
    sum[2] += (x**n)/n

print('\n', sum[2])

for i in range(1, n):
    print((-x**n)/n, end=' ')
    sum[3] += (-x**n)/n

print('\n', sum[3])