all:
	g++ -std=gnu++0x server.cpp -o stringServer -g -pthread
	g++ -std=gnu++0x client.cpp -o stringClient -g -pthread

clean:
	rm stringServer stringClient

# gcc -c -o out.o out.c
# -c means to create an intermediary object file, rather than an executable.
# 
# ar rcs libout.a out.o
# This creates the static library. r means to insert with replacement, 
# c means to create a new archive, and s means to write an index. 
# As always, see the man page for more info.
# 
