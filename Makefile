all:
	g++ -std=gnu++0x server.cpp -o stringServer -g -pthread
	g++ -std=gnu++0x client.cpp -o stringClient -g -pthread

clean:
	rm stringServer stringClient
