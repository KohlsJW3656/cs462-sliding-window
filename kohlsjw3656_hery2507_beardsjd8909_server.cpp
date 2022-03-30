#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>



using namespace std;

int server() {

    //this is memory space set aside for storing packets awaiting transmission over networks or storing packets received over networks.
    char packetBuffer[1025];
    struct sockaddr_in serverIpAddress;

    time_t clock;
    int clientListener = 0;
    int clientConnection = 0;

    //create socket
    clientListener = socket(AF_INET, SOCK_STREAM, 0);
    if (clientListener < 0){
        perror("ERROR cannot open socket");
    }
    memset(&serverIpAddress, '0', sizeof(serverIpAddress));
    memset(packetBuffer, '0', sizeof(packetBuffer));

    serverIpAddress.sin_family = AF_INET;
    serverIpAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverIpAddress.sin_port = htons(8080);
    bind(clientListener, (struct sockaddr*)&serverIpAddress, sizeof(serverIpAddress));
    listen(clientListener , 20);

    while(true){
        cout<<"running server, client requested"<<endl;
        clientConnection = accept(clientListener, (struct sockaddr*)NULL, NULL);

        clock = time(NULL);
        snprintf(packetBuffer, sizeof(packetBuffer), "%.24s\r\n", ctime(&clock));
        write(clientConnection, packetBuffer, strlen(packetBuffer));

        close(clientConnection);
        sleep(1);
    }

    return 0;
}