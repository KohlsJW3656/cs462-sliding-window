
//#define DEBUG

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

struct hdr {
  int seq;
  uint32_t checkSum;
  bool ack;
  bool sent;
  bool retransmitted;
  clock_t sentTime;
  unsigned int dataSize;
};

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
  int retransmittedCounter = 0;
  int originalCounter = 0;
  int errorInput;
  int randInput;
  int errorPacket;
  int windowStart = 0;
  int windowEnd = slidingWindowSize - 1;
  clock_t startTime;
  unsigned int throughput = 0;

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
  startTime = clock();

  /* While we haven't reached the end of the file */
  while(!feof(file)) {
    /* While our sliding window hasn't been filled up */
    while (slidingWindow.size() < slidingWindowSize) {
      packet = (char*) malloc(packetSize + 1);
      unsigned int dataSize = fread(packet + sizeof(struct hdr), 1, packetSize - sizeof(struct hdr), file);
      /* If reading successful, push packet to front and increase the sequence */
      if (dataSize) {
        throughput += dataSize + + sizeof(struct hdr);
        slidingWindow.push_front(packet);
        uint32_t checkSum = GetCrc32(packet + sizeof(struct hdr), dataSize);
        auto *packetHeader = (struct hdr *) packet;
        packetHeader->seq = packetSeqCounter;
        packetHeader->checkSum = checkSum;
        packetHeader->dataSize = dataSize;
        packetHeader->ack = false;
        packetSeqCounter++;
      }
    }
    /* Send all packets in our slidingWindow */
    for (auto i = slidingWindow.rbegin(); i != slidingWindow.rend(); ++i) {
      /* If we haven't sent the packet and haven't acked */
      if (!getHeader(*i)->sent) {
        cout << "Packet " << getHeader(*i)->seq << " sent" << endl;
        getHeader(*i)->sent = true;
        getHeader(*i)->sentTime = clock();
        originalCounter++;
        #ifdef DEBUG
          printPacket(*i, getHeader(*i)->dataSize);
        #endif
        send(sock, *i, getHeader(*i)->dataSize + sizeof(struct hdr), 0);
      }
      /* If the duration is greater than timeout interval, and we haven't acked the packet, resend */
      if (((double)(clock() - getHeader(*i)->sentTime) / (double) ((double) CLOCKS_PER_SEC / 1000)) > timeoutInterval && !getHeader(*i)->ack && getHeader(*i)->sent) {
        cout << "Packet " << getHeader(*i)->seq << " *****Timed Out *****" << endl;
        cout << "Packet " << getHeader(*i)->seq << " Re-transmitted." << endl;
        getHeader(*i)->sentTime = clock();
        getHeader(*i)->retransmitted = true;
        retransmittedCounter++;
        send(sock, *i, getHeader(*i)->dataSize + sizeof(struct hdr), 0);
      }
    }

    /* Responses */
    while (!slidingWindow.empty()) {
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
        }
      }
    }
  }
  double elapsedTime = (double)(clock() - startTime) / (double) CLOCKS_PER_SEC;

  cout << "Session successfully terminated" << endl << endl;
  cout << "Number of original packets sent: " << originalCounter << endl;
  cout << "Number of retransmitted packets sent: " << retransmittedCounter << endl;
  cout << "Total elapsed time: " << elapsedTime << endl;
  cout << "Total throughput (Mbps): " << throughput / elapsedTime << endl;
  cout << "Effective throughput: " << throughput;

  fclose(file);
  close(sock);
  return 0;
}
