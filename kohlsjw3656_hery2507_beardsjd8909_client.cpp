#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

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

int client(char* ip, int port, int protocol, int packetSize, int timeoutType, int timeoutInterval, int multiFactor, int slidingWindowSize, int seqStart, int seqEnd, int userType) {
    int obj_socket = 0, reader;
    struct sockaddr_in serv_addr;
    char *message = "A message from Client !";
    char buffer[1024] = {0};
    if (( obj_socket = socket (AF_INET, SOCK_STREAM, 0 )) < 0)
    {
      printf ( "Socket creation error !" );
      return -1;
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
// Converting IPv4 and IPv6 addresses from text to binary form
    if(inet_pton ( AF_INET, ip, &serv_addr.sin_addr)<=0)
    {
      printf ( "\nInvalid address ! This IP Address is not supported !\n" );
      return -1;
    }
    if ( connect( obj_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr )) < 0)
    {
      printf ( "Connection Failed : Can't establish a connection over this socket !" );
      return -1;
    }
    send ( obj_socket , message , strlen(message) , 0 );
    printf ( "\nClient : Message has been sent !\n" );
    reader = read ( obj_socket, buffer, 1024 );
    printf ( "%s\n",buffer );
    return 0;
  }
