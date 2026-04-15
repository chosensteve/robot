import pymysql as sq
import sys 

user_v = 'root'
password_v = '1108'
data_base = ''
sel_table = ''


def connect():
    try:
        q = sq.connect(host='localhost', user=user_v, password=password_v, charset='UTF8', db=data_base)
        cur = q.cursor()
    except Exception as e:
        print(e)
        sys.exit()
    else:
        return q, cur

def close(q, cur):
    q.close()
    cur.close()



def login():
    global user_v 
    global password_v
    while True:
        user_v = input("username: ")
        password_v = input("password: ")
        try:
            q = sq.connect(host='localhost', user=user_v, password=password_v, charset='UTF8')
        except Exception as n:
            print("Incorrect Username or Password")
        else:
            break
    sql_menu()


def sql_menu():
    
    
    while True:
        print("\n0. Exit")
        print("1. Show Databases")
        print("2. Create Database")
        print("3. Delete Database")
        print("4. Select Database")
        print()
        
        c = int(input("\tchoice: "))
        print()
        
        try:
            q, cur = connect()

            if c == 0:
                sys.exit()
            
            if c == 1:
                r = cur.execute("show databases;")
                for i in cur.fetchall():
                    print(i[0], end='\n\n')
            
            if c == 2:
                r = cur.execute(f"create database {input("Database name: ")};")
            
            if c == 3:
                r =  cur.execute(f"drop database {input("Database name: ")};")
            
            if c == 4:
                global data_base
                data_base = input("Database name: ")
                r = cur.execute(f"use {data_base};")
                close(q, cur)
                db_menu()

        except Exception as n:
            print(n)
        finally:
            close(q, cur)


def db_menu():
    print("\n0. Exit")
    print("1. Show Tables")
    print("2. Setup Tables")
    print("3. Create Custom Table")
    print("4. Delete Table")
    print("5. Select Table")

    c = input("\tChoice: ")
    print()
    
    try:
        q, cur = connect()
    
        if c == 0:
            sys.exit() 
        if c == 1:
            r = cur.execute("show tables;")
            for i in cur.fetchall():
                print(i[0])
        if c == 2:
            close(q, cur)
            setup_tables()
        if c == 3:
            print("Work In Progress")
        if c == 4:
            r =  cur.execute(f"drop table {input("Table name: ")};")
        #if c == 5:
    except Exception as n:
        print(n)
    finally:
        close(q, cur)   



#login()
sql_menu()