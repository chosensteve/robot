x = input("").split()
n = int(x[0])
k = int(x[1])
p = int(x[2])

l = list(map(int, input("").split()))  
plist = [list(map(int, input("").split())) for _ in range(p)]

f = sorted([(l[i], i) for i in range(n)]) 

# Step 2: Build component IDs
c_id = [0] * n
cur_c = 0
c_id[f[0][1]] = cur_c

for i in range(1, n):
    # If distance <= k, same component
    if f[i][0] - f[i - 1][0] <= k:
        c_id[f[i][1]] = cur_c
    else:
        # Otherwise start a new component
        cur_c += 1
        c_id[f[i][1]] = cur_c

# Step 3: Answer queries
for a, b in plist:
    if c_id[a - 1] == c_id[b - 1]:
        print("YES")
    else:
        print("NO")