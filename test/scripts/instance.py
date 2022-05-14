import random

a = 64 * 24
b = [(2, 5), (4, 8), (4, 12), (4, 18), (8, 18), ]

s = {}

while a > 10:
    j = random.choice (b)
    print (str (a) + " " + str (j))
    if (a > (j[0] * j[1])) :
        if j in s :
            s[j] = s[j] + 1
        else : 
            s [j] = 1
        a = a - (j[0] * j[1])
            
print (s)
