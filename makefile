run : menu.o client.o server.o
	g++ menu.o client.o server.o -o menu

menu.o: kohlsjw3656_hery2507_beardsjd8909_menu.cpp
	g++ -g -c kohlsjw3656_hery2507_beardsjd8909_menu.cpp -o menu.o

client.o: kohlsjw3656_hery2507_beardsjd8909_client.cpp
	g++ -g -c kohlsjw3656_hery2507_beardsjd8909_client.cpp -o client.o

server.o: kohlsjw3656_hery2507_beardsjd8909_server.cpp
	g++ -g -c kohlsjw3656_hery2507_beardsjd8909_server.cpp -o server.o

clean:
	rm menu *.o