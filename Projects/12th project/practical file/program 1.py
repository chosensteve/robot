#Write a function that takes an integer as an argument and returns the sum of its individual digits.

def digit_sum(n):
    s = 0
    if n < 0:
        n = -n
    for i in range(len(str(n))) :
        s = s + n%10
        n = int(n/10)
    
    print(s)


x = 0
digit_sum(x)