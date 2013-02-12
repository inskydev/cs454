all:
	g++ server.cpp -o stringServer
	g++ client.cpp -o stringClient

clean:
	rm stringServer stringClient
