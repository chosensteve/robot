x = input("num: ")
y = int(x)
print(len(x))
sum, sum1 = 0, 0

for i in range(1,y):
    if y%i == 0:
        sum += i

if sum == y:
    print("perfect")

for i in x:
    sum1 += int(i)**len(x)

if sum1 == int(x):
    print("armstrong")

if x == x[::-1]:
    print("palindrome")