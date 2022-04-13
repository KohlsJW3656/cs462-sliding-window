//
// Created by Jonas Kohls on 03/11/2022.
//

//#define DEBUG

#include <bits/stdc++.h>
#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string>
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

uint32_t GetCrc32Server(char *data, unsigned int dataSize) {
  boost::crc_32_type result;
  result.process_bytes(data, dataSize);
  return result.checksum();
}

struct hdr* getHeaderServer(char* packet) {
  return (struct hdr*) packet;
}

void printPacketServer(char* packet, int packetSize) {
  cout << "Seq: " << getHeaderServer(packet)->seq << endl;
  cout << "Checksum: " << getHeaderServer(packet)->checkSum << endl;
  cout << "Ack: " << getHeaderServer(packet)->ack << endl;
  cout << "Data Size: " << getHeaderServer(packet)->dataSize << endl;

  for (int i = sizeof(struct hdr); i < packetSize; i++) {
    printf("%02X ", packet[i]);
  }
  cout << "\n";
}

string printSlidingWindowServer(int start, int end) {
  string slidingWindow = "Current window = [";
  for (int i = start; i < end; i++) {
    slidingWindow+= to_string(i) + ", ";
  }
  return slidingWindow += to_string(end) + "]";
}

int server(int port, int protocol, int packetSize, int slidingWindowSize, int seqEnd, int errors) {
  list<char*> slidingWindow;
  char* packet;
  int listening = socket(AF_INET, SOCK_STREAM, 0);
  if (listening == -1) {
    cerr << "Failed to create socket!" << endl;
    return -1;
  }
  int windowStart = 0;
  int windowEnd = slidingWindowSize - 1;
  int lastSeq = 0;
  int retransmittedCounter = 0;
  int originalCounter = 0;
  srand (time(NULL));
  int flag = 0;

  /* Bind the ip address and port to a socket */
  sockaddr_in hint;
  hint.sin_family = AF_INET;
  hint.sin_port = htons(port);
  inet_pton(AF_INET, "0.0.0.0", &hint.sin_addr);

  bind(listening, (sockaddr*)&hint, sizeof(hint));

  listen(listening, SOMAXCONN);
  cout << "Server running on port " << port << endl;

  /* Wait for a connection */
  sockaddr_in client;
  socklen_t clientSize = sizeof(client);

  int clientSocket = accept(listening, (sockaddr*)&client, &clientSize);

  char host[NI_MAXHOST];      // Client's remote name
  char service[NI_MAXSERV];   // Service (i.e. port) the client is connected on

  memset(host, 0, NI_MAXHOST);
  memset(service, 0, NI_MAXSERV);

  if (getnameinfo((sockaddr*)&client, sizeof(client), host, NI_MAXHOST, service, NI_MAXSERV, 0) == 0) {
    cout << host << " connected on port " << service << endl;
  }
  else {
    inet_ntop(AF_INET, &client.sin_addr, host, NI_MAXHOST);
    cout << host << " connected on port " << ntohs(client.sin_port) << endl;
  }

  /* Close listening socket */
  close(listening);

  while (true) {
    /* While we can receive packets */
    packet = (char*) malloc(packetSize + 1);
    int bytesReceived = recv(clientSocket, packet, packetSize, 0);
    if (bytesReceived == -1 || bytesReceived == 0) {
      break;
    }
    #ifdef DEBUG
      cout << "Received: " << bytesReceived << endl;
      printPacketServer(packet, packetSize);
    #endif

    /* If the packet fits within the sliding window */
    if (getHeaderServer(packet)->seq >= windowStart && getHeaderServer(packet)->seq <= windowEnd) {
      cout << "Packet " << getHeaderServer(packet)->seq << " received" << endl;
      lastSeq = getHeaderServer(packet)->seq;
      slidingWindow.push_front(packet);
      /* Count packet */
      if (getHeaderServer(packet)->retransmitted) {
        retransmittedCounter++;
      }
      else {
        originalCounter++;
      }
      /* Calculate the Checksum */
      uint32_t checkSum = GetCrc32Server(packet + sizeof(struct hdr), getHeaderServer(packet)->dataSize);
      flag = (rand()% 20) + 1;
      if (errors == 2 && flag == 4) {
        cout << "Packet " << getHeaderServer(packet)->seq << " dropped" << endl;
        send(clientSocket, packet, packetSize, 0);
      }
      else if (checkSum == getHeaderServer(packet)->checkSum) {
        getHeaderServer(packet)->ack = true;
        cout << "Checksum okay" << endl;
        cout << "Ack " << getHeaderServer(packet)->seq << " sent" << endl;
        flag = (rand()% 20) + 1;
        if (errors == 2 && flag == 4) {
          cout << "Ack " << getHeaderServer(packet)->seq << " lost" << endl;
          getHeaderServer(packet)->ack = false;
          send(clientSocket, packet, packetSize, 0);
        }
        else {
          send(clientSocket, packet, packetSize, 0);
          windowStart++;
          windowEnd++;
          cout << printSlidingWindowServer(windowStart, windowEnd) << endl;
        }
      }
      else {
        cout << "Checksum failed" << endl;
        send(clientSocket, packet, packetSize, 0);
        cout << printSlidingWindowServer(windowStart, windowEnd) << endl;
      }
      slidingWindow.pop_back();
    }
    /* If we are using SR and got a packet out of order, we will send a negative ack back */
    else if (protocol == 2) {
      cout << "Packet " << getHeaderServer(packet)->seq << " received" << endl;
      cout << "Packet " << getHeaderServer(packet)->seq <<  " nack" << endl;
      send(clientSocket, packet, packetSize, 0);
    }
  }
  cout << "Last packet seq# received: " << lastSeq << endl;
  cout << "Number of original packets received: " << originalCounter << endl;
  cout << "Number of retransmitted packets received: " << retransmittedCounter << endl;
  /* Close the socket */
  close(clientSocket);
  return 0;
}
