all:
	g++ -std=gnu++0x server.cpp -o stringServer
	g++ client.cpp -o stringClient

clean:
	rm stringServer stringClient
