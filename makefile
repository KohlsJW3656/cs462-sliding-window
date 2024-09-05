BOOST_ROOT := boost
BOOST_INC := ${BOOST_ROOT}/include

run : menu.o sender.o receiver.o
	g++ -I$(BOOST_ROOT) menu.o sender.o receiver.o -o menu

menu.o: menu.cpp
	g++ -g -c menu.cpp -o menu.o

sender.o: sender.cpp
	g++ -g -c sender.cpp -o sender.o

receiver.o: receiver.cpp
	g++ -g -c receiver.cpp -o receiver.o

clean:
	rm menu *.o

