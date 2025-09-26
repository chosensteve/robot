lst = []
for i in range(1, 4):
    lst.append(int(input(f"num{i}: ")))

lst.sort(reverse=True)
print(lst[0])