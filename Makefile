
all: lib
	# A Server
	g++ -std=gnu++0x -L. server.cpp -lrpc -o server 
	# A client
	g++ -std=gnu++0x -L. client.cpp -lrpc -lpthread -o client 
	# Binder
	g++ -std=gnu++0x -L. binder_main.cpp -lrpc -o binder

lib:
	# Static library
	g++ -std=gnu++0x -c -g -o util.o util.cpp
	g++ -std=gnu++0x -c -g -o Transporter.o Transporter.cpp
	g++ -std=gnu++0x -c -g -o Binder.o Binder.cpp
	ar rcs librpc.a util.o Transporter.o Binder.o


clean:
	rm -f rpc.o librpc.a

# gcc -c -o out.o out.c
# -c means to create an intermediary object file, rather than an executable.
# 
# ar rcs libout.a out.o
# This creates the static library. r means to insert with replacement, 
# c means to create a new archive, and s means to write an index. 
# As always, see the man page for more info.
# 
