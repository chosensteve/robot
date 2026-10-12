#Write a menu-driven program allowing the user to insert, delete, search, and display elements in a list.

l = []

def insert():
    x = input("Data to insert: ")
    l.append(x)

def delete():
    x = input("Data to delete: ")
    if x in l:
        l.remove(x)
        print(x, 'was deleted')
    else:
        print("element", x ,"is not present in the list")

def search():
    x = input("data to search: ")
    for i in range(len(l)):
        if l[i] == x:
            print("element", i+1, "is", x)
            return
    print("element", x ,"is not present in the list")

def display():
    for i in range(len(l)):
        print(i+1, l[i])

while True:
    print('0.Exit')
    print('1.Display list')
    print('2.Insert element')
    print('3.Search element')
    print('4.Delte element')

    n = int(input("choice: "))
    print()

    if n == 0:
        break
    elif n == 1:
        display()
    elif n == 2:
        insert()
    elif n == 3:
        search()
    elif n == 4:
        delete()
    else:
        print('invalid choice\n')

    print()