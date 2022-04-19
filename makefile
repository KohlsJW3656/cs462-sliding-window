BOOST_ROOT := boost
BOOST_INC := ${BOOST_ROOT}/include

run : menu.o sender.o receiver.o
	g++ -I$(BOOST_ROOT) menu.o sender.o receiver.o -o menu

menu.o: kohlsjw3656_hery2507_beardsjd8909_menu.cpp
	g++ -g -c kohlsjw3656_hery2507_beardsjd8909_menu.cpp -o menu.o

sender.o: kohlsjw3656_hery2507_beardsjd8909_sender.cpp
	g++ -g -c kohlsjw3656_hery2507_beardsjd8909_sender.cpp -o sender.o

receiver.o: kohlsjw3656_hery2507_beardsjd8909_receiver.cpp
	g++ -g -c kohlsjw3656_hery2507_beardsjd8909_receiver.cpp -o receiver.o

clean:
	rm menu *.o
