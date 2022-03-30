#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>



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

    struct sockaddr_in serverIpAddress;
    int sock_fd = 0;
    int n = 0;
    char packetBuffer[1024];

    memset(packetBuffer, '0', sizeof(packetBuffer));
    serverIpAddress.sin_family = AF_INET;
    serverIpAddress.sin_port = htons(port);
    serverIpAddress.sin_addr.s_addr = inet_addr(ip);

    if (connect(sock_fd, (struct sockaddr*)&serverIpAddress, sizeof(serverIpAddress)) < 0){
        cout<<"connect fail because port and ip issue"<<endl;
        return 1;
    }

    while((n = read(sock_fd, packetBuffer, sizeof(packetBuffer)) - 1) > 0){
        packetBuffer[n] = 0;
        if (fputs(packetBuffer, stdout) == EOF){
            cout<<"there is standard error"<<endl;
        }

    }
    if(n < 0){
        cout<<"There is standard error"<<endl;
    }
    return 0;
}
