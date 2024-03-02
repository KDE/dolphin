import fileinput

for line in fileinput.input():
    if line.startswith("DirectorySizeCount=true"):
        print("DirectorySizeMode=ContentCount")
    if line.startswith("DirectorySizeCount=false"):
        print("DirectorySizeMode=ContentSize")

print("# DELETE DirectorySizeCount")

