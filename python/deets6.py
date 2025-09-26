num = [int(input("x: ")), int(input("y: "))]
num.sort()
x = num[0]
y = num[1]
div = []

for i in range(1, x+1):
    if x%i == 0 and y%i == 0:
        div.append(i)

div.sort()
print("HCF: ", div[-1])

print("lcm: ", int((x*y)/div[-1]))
