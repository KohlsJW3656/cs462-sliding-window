//
// Created by Jonas Kohls on 03/11/2022.
//

#define DEBUG

#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <stdlib.h>
using namespace std;

typedef u_char Sequence;
typedef char Msg;
typedef char Event;

struct Semaphore {

    int mutex;
    int rcount; //number of readers
    int rwait; //number of readers waiting
    int wrt; //boolean to check if write is in progress

};

typedef struct {

    Sequence Num; //the sequence number for the frame
    Sequence AckNum; //the acknowlege number for the received frame
    u_char Flags; //the flags used


} Header;

typedef struct {

    //SENDER
    Sequence prevACK; // last ack received
    Sequence prevFrame; // last frame received
    Header header; // pre-initialized header
    Semaphore sendWindowNotFull; //semaphore to see if send window is full

    struct send_slot {
        Event timeout;
        Msg  msg;

    };

    //RECEIVER
    Sequence nextFrame; //sequence number for next frame expected

    struct receiver_slot {

        int received; //checks if the received message is valid
        Msg msg;

    };

} State;

int server(int port, int protocol, int packetSize, int timeoutType, int timeoutInterval, int multiFactor, int slidingWindowSize, int seqStart, int seqEnd, int userType) {
    int obj_server, sock, reader;
    struct sockaddr_in address;
    int opted = 1;
    int address_length = sizeof(address);
    char buffer[packetSize];
    char *message = "A message from server !";
    if (( obj_server = socket ( AF_INET, SOCK_STREAM, 0)) == 0)
    {
      perror ( "Opening of Socket Failed !");
      exit ( EXIT_FAILURE);
    }
    if ( setsockopt(obj_server, SOL_SOCKET, SO_REUSEADDR,
                      &opted, sizeof ( opted )))
    {
      perror ( "Can't set the socket" );
      exit ( EXIT_FAILURE );
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( port );
    if (bind(obj_server, ( struct sockaddr * )&address,
             sizeof(address))<0)
    {
      perror ( "Binding of socket failed !" );
      exit(EXIT_FAILURE);
    }
    if (listen ( obj_server, 3) < 0)
    {
      perror ( "Can't listen from the server !");
      exit(EXIT_FAILURE);
    }
    if ((sock = accept(obj_server, (struct sockaddr *)&address, (socklen_t*)&address_length)) < 0)
    {
      perror("Accept");
      exit(EXIT_FAILURE);
    }
    reader = read(sock, buffer, packetSize);
    printf("%s\n", buffer);
    send(sock , message, strlen(message) , 0 );
    printf("Server : Message has been sent ! \n");
    return 0;
  }
