t=int(input())
for _ in range(t):
    a,b,k=map(int,input().split())
    s=0
    for x in range(a,b+1):
        c=0
        y=x
        for i in range(2,y+1):
            if y%i==0:
                f=0
                for j in range(2,int(i**0.5)+1):
                    if i%j==0:f=1;break
                if f==0:
                    c+=1
                    while y%i==0:y//=i
        if c==k:s+=1
    print(s)
