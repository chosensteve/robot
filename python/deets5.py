fib = [0, 1]
n = int(input("hi: "))
for i in range(n):
    print(fib[0], end=' ')
    fib[1], fib[0] = fib[0] + fib[1], fib[1]

print()