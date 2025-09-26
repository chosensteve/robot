students = {
    1: {"name": "Alice", "marks": 88},
    2: {"name": "Bob", "marks": 92},
    3: {"name": "Charlie", "marks": 75},
    4: {"name": "Diana", "marks": 81},
    5: {"name": "Ethan", "marks": 95},
    6: {"name": "Fiona", "marks": 78},
    7: {"name": "George", "marks": 84}
}

for i in range(1, len(students)+1):
    if students[i]["marks"] > 75:
        print(students[i]["name"])
