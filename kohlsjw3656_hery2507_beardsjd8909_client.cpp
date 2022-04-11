#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <list>
#include "boost/crc.hpp"

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

struct hdr {
  int seq;
  uint32_t checkSum;
  bool ack;
  unsigned int dataSize;
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

uint32_t GetCrc32(char *data, unsigned int dataSize) {
  boost::crc_32_type result;
  result.process_bytes(data, dataSize);
  return result.checksum();
}

struct hdr* getHeader(char* packet) {
  return (struct hdr*) packet;
}

void printPacket(char* packet, int packetSize) {
  cout << "Seq: " << getHeader(packet)->seq << endl;
  cout << "CheckSum: " << getHeader(packet)->checkSum << endl;
  cout << "Ack: " << getHeader(packet)->ack << endl;
  cout << "Data Size: " << getHeader(packet)->dataSize << endl;

  for (int i = sizeof(struct hdr); i < packetSize; i++) {
    printf("%02X ", packet[i]);
  }
  cout << "\n";
}

string printSlidingWindow(int start, int end) {
  string slidingWindow = "Current window = [";
  for (int i = start; i < end; i++) {
    slidingWindow+= to_string(i) + ", ";
  }
  return slidingWindow += to_string(end) + "]";
}

int client(string ip, int port, int protocol, int packetSize, int timeoutType, int timeoutInterval, int multiFactor, int slidingWindowSize, int seqEnd, int userType) {
  char *filename = (char*)malloc(20 * sizeof(char));
  FILE *file;
  char* packet;
  list<char*> slidingWindow;
  string userInput;
  int packetSeqCounter = 0;
  int packetLoopCounter = 0;
  int errorInput;
  int randInput;
  int errorPacket;
  int windowStart = 0;
  int windowEnd = slidingWindowSize - 1;

  do {
//    cout << "Would you like to have errors? 1 yes 2 no\n";
//    cin >> errorInput;
//
//    if (errorInput == 1) {
//      cout << "Would you like random errors? 1 yes 2 no\n";
//      cin >> randInput;
//
//      if (randInput == 1) {
//        int randNum = (rand()%packetSize);
//        errorPacket = randNum;
//      }
//      else {
//        cout << "Enter the packet to error: \n";
//        cin >> errorPacket;
//      }
//    }
    cout << "Please enter the file name: ";
    cin >> filename;
    file = fopen(filename, "rb");
    if (!file) {
      cout << "Invalid filename\n";
    }
  }
  while (!file);

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
    return -1;
  }

  /* While we haven't reached the end of the file */
  while(!feof(file)) {
    /* While our sliding window hasn't been filled up */
    while (slidingWindow.size() < slidingWindowSize) {
      packet = (char*) malloc(packetSize + 1);
      unsigned int dataSize = fread(packet + sizeof(struct hdr), 1, packetSize - sizeof(struct hdr), file);
      /* If reading successful, push packet to front and increase the sequence */
      if (dataSize) {
        slidingWindow.push_front(packet);
        uint32_t checkSum = GetCrc32(packet + sizeof(struct hdr), dataSize);
        auto *packetHeader = (struct hdr *) packet;
        packetHeader->seq = packetSeqCounter;
        packetHeader->checkSum = checkSum;
        packetHeader->dataSize = dataSize + 1;
        packetHeader->ack = false;
        packetSeqCounter++;
      }
    }
    /* Send all packets in our slidingWindow */
    for (auto i = slidingWindow.rbegin(); i != slidingWindow.rend(); ++i) {
      /* Send to server */
      cout << "Packet " << getHeader(*i)->seq << " sent" << endl;
      //printPacket(*i, getHeader(*i)->dataSize);
      int sendRes = send(sock, *i, getHeader(*i)->dataSize + sizeof(struct hdr), 0);
      if (sendRes == -1) {
        cout << "Failed to send packet to server!\r\n";
        break;
      }
    }

    /* Responses */
    for (auto i = slidingWindow.rbegin(); i != slidingWindow.rend(); ++i) {
      /* Packet header responses */
      packet = (char*) malloc(sizeof(struct hdr));
      int bytesReceived = recv(sock, packet, sizeof(struct hdr), 0);
      if (bytesReceived == -1) {
        cout << "There was an error getting response from server\r\n";
      }
      else {
        if (getHeader(packet)->ack) {
          cout << "Ack " << getHeader(packet)->seq << " received" << endl;
          /* If this is the correct packet we were looking for */
          if (getHeader(packet)->seq == getHeader(slidingWindow.back())->seq) {
            slidingWindow.pop_back();
            windowStart++;
            windowEnd++;
            cout << printSlidingWindow(windowStart, windowEnd) << endl;
          }
          else {
            //TODO put packet on another list of acked packets
          }
        }
        else {
          //TODO check for time outs
          cout << "Ack was false" << endl;
          //printPacket(packet, packetSize);
        }
      }
    }
  }
  fclose(file);

//  if (1/*PACKET IS MISSING*/) {
//    if (protocol == 2) {
//      int previousPacketReceived = 0/*the previous packet received in order*/;
//      int nextPacketExpected = 0/*the next packet expected given the last packet received*/;
//      if (nextPacketExpected - previousPacketReceived != 1) {
//        int missingPackets = nextPacketExpected - previousPacketReceived;
//        for (int i = 1; i < missingPackets - 2; i++) {
//          int currentMissingNum = previousPacketReceived + i;
//          //send the missing packet previousPacketReceived + i
//        }
//
//      }
//      else {
//        //send the missing packet
//      }
//    }
//  }
//  while(true);
  //	Close the socket
  close(sock);
  return 0;
}
