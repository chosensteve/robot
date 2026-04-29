x = int(input("num: "))
div = 0

for i in range(1, x):
    if x%i == 0:
        div += 1
    if div > 1:
        print("composite num")
        break

if div == 1:
    print("primer")