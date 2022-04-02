#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string>
#include <bits/stdc++.h>
#include <cstdio>
#include <cstdlib>

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

int client(string ip, int port, int protocol, int packetSize, int timeoutType, int timeoutInterval, int multiFactor, int slidingWindowSize, int seqEnd, int userType) {
  char *filename = (char*)malloc(20 * sizeof(char));
  FILE *file;
  char* packet;
  vector<char**> slidingWindow;
  string userInput;
  int packetSeqCounter = 0;

  do {
    cout << "Please enter the file name: ";
    cin >> filename;
    file = fopen(filename, "rb");
    if (!file) {
      cout << "Invalid filename\n";
    }
  }
  while (!file);

  /* TODO while things to read */
  while(true) {
    packet = (char*) malloc(packetSize + 1);
    unsigned int num = fread(packet + 20, 1, packetSize - 20, file);
    if (num) {
      //packet[0] = (char)packetSeqCounter;
      cout << num << "\n";
      for (int i = 0; i < packetSize; i++) {
        cout << packet[i];
      }
      slidingWindow.push_back(&packet);
      cout << "\n";
      packetSeqCounter++;
    }
    break; //TODO remove
  }
  fclose(file);

  for (int i = 0; i < slidingWindow.size(); i++) {
    cout << slidingWindow[i] << "\n";
    for (int j = 0; j < packetSize; j++) {
      cout << *slidingWindow[i][j];
    }
    cout << "\n";
  }

  //	Create a socket
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock == -1) {
    cout << "Failed to create socket\n";
    return 1;
  }

  sockaddr_in hint;
  hint.sin_family = AF_INET;
  hint.sin_port = htons(port);
  inet_pton(AF_INET, ip.c_str(), &hint.sin_addr);

  //	Connect to the server on the socket
  int connectRes = connect(sock, (sockaddr*)&hint, sizeof(hint));
  if (connectRes == -1) {
    cout << "Failed to connect to server\n";
    return 1;
  }


  while (false) {
    //TODO Assumes header size of 20 (temp for now)
    //fread(data + 20, 1, packetSize - 20, file);
    //TODO Populate header packet[0] = whatever
    //		Enter lines of text
    cout << "> ";
    //getline(cin, userInput);

    //		Send to server
    int sendRes = send(sock, userInput.c_str(), userInput.size() + 1, 0);
    if (sendRes == -1) {
      cout << "Could not send to server! Whoops!\r\n";
      continue;
    }

    //		Wait for response
    //memset(slidingWindow, 0, slidingWindowSize);
    int bytesReceived; // = recv(sock, slidingWindow, slidingWindowSize, 0);
    if (bytesReceived == -1) {
      cout << "There was an error getting response from server\r\n";
    }
    else {
      //		Display response
      cout << "SERVER> " << /*string(slidingWindow, bytesReceived) << */"\r\n";
    }

    if (0/*PACKET IS MISSING*/) {

        if (protocol == 2) {

            int previousPacketReceived = 0/*the previous packet received in order*/;
            int nextPacketExpected = 0/*the next packet expected given the last packet received*/;

            if (nextPacketExpected - previousPacketReceived != 1) {

                int missingPackets = nextPacketExpected - previousPacketReceived;

                for (int i = 1; i < missingPackets - 2; i++) {

                    int currentMissingNum = previousPacketReceived + i;

                    //send the missing packet previousPacketReceived + i

                }

            } else {

                //send the missing packet

            }

        }

    }

  } while(true);

  //	Close the socket
  close(sock);

  return 0;
}
