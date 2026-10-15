import random
import string

n = int(input("Enter the number of letters: "))

# Generates a list of n random lowercase and uppercase letters
# Use string.ascii_lowercase if you only want lowercase letters
random_letters = random.choices(string.ascii_letters, k=n)

print(random_letters)

