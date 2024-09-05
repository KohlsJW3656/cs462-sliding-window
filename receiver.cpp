//
// Created by Jonas Kohls on 03/11/2022.
//

//#define DEBUG

#include <bits/stdc++.h>
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string>
#include "boost/crc.hpp"
using namespace std;

struct hdr {
  int seq;
  unsigned int dataSize;
  uint32_t checkSum;
  bool ack;
  bool sent;
  bool retransmitted;
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
  cout << endl;
}

string printSlidingWindowServer(int wrappingMode, int windowStart, int windowEnd, int seqEnd) {
  string slidingWindow = "Current window = [";
  /* If the window is not wrapping */
  if (!wrappingMode) {
    for (int i = windowStart; i < windowEnd; i++) {
      slidingWindow+= to_string(i) + ", ";
    }
  }
  else {
    for (int i = windowStart; i <= seqEnd; i++) {
      slidingWindow+= to_string(i) + ", ";
    }
    for (int j = 0; j < windowEnd; j++) {
      slidingWindow+= to_string(j) + ", ";
    }
  }
  return slidingWindow += to_string(windowEnd) + "]";
}

bool packetCanFit(int wrappingMode, int packetSeq, int windowStart, int windowEnd) {
  /* If the window is not wrapping */
  if (!wrappingMode) {
    return packetSeq >= windowStart && packetSeq <= windowEnd;
  }
  else {
    return packetSeq >= windowStart || packetSeq <= windowEnd;
  }
}

int receiver(int port, int protocol, int packetSize, int slidingWindowSize, int seqEnd, int errors) {
  FILE *file;
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
  bool wrappingMode = false;
  list<int> packetsToDrop;
  list<int> acksToLose;
  int numPacketsToDrop;
  int numAcksToLose;

  if (errors == 3) {
    cout << "Number of packets to drop: ";
    cin >> numPacketsToDrop;
    for (int i = 0; i < numPacketsToDrop; i++) {
      cout << "Sequence to drop: ";
      int packToDrop;
      cin >> packToDrop;
      packetsToDrop.push_front(packToDrop);
    }
    cout << "Number of acks to lose: ";
    cin >> numAcksToLose;
    for (int i = 0; i < numAcksToLose; i++) {
      cout << "Sequence to lose: ";
      int lostAck;
      cin >> lostAck;
      acksToLose.push_front(lostAck);
    }
    packetsToDrop.sort(std::greater<>());
    acksToLose.sort(std::greater<>());
  }

  /* Bind the ip address and port to a socket */
  sockaddr_in hint;
  hint.sin_family = AF_INET;
  hint.sin_port = htons(port);
  inet_pton(AF_INET, "0.0.0.0", &hint.sin_addr);

  bind(listening, (sockaddr*)&hint, sizeof(hint));

  listen(listening, SOMAXCONN);
  cout << "Receiver running on port " << port << endl;

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

  file = fopen("/tmp/kohls-out", "w");

  while (true) {
    packet = (char*) malloc(packetSize + 1);
    int bytesReceived = 0;
    int totalBytes = 0;
    bytesReceived = recv(clientSocket, packet, packetSize, 0);
    totalBytes = bytesReceived;
    /* Continue to read in bytes until we get the full packet */
    while (totalBytes != getHeaderServer(packet)->dataSize + sizeof(struct hdr) && totalBytes != packetSize) {
      bytesReceived = recv(clientSocket, packet + totalBytes, getHeaderServer(packet)->dataSize + sizeof(hdr) - totalBytes, 0);
      if (bytesReceived == -1 || bytesReceived == 0) {
        break;
      }
      totalBytes += bytesReceived;
    }
    if (bytesReceived == -1 || bytesReceived == 0) {
      break;
    }
    #ifdef DEBUG
      cout << "Received: " << bytesReceived << endl;
      printPacketServer(packet, packetSize);
    #endif

    /* If the packet fits within the sliding window */
    if (packetCanFit(wrappingMode, getHeaderServer(packet)->seq, windowStart, windowEnd)) {
      cout << "Packet " << getHeaderServer(packet)->seq << " received" << endl;
      lastSeq = getHeaderServer(packet)->seq;
      /* Count packet */
      if (getHeaderServer(packet)->retransmitted) {
        retransmittedCounter++;
      }
      else {
        originalCounter++;
      }
      /* Calculate the Checksum */
      uint32_t checkSum = GetCrc32Server(packet + sizeof(struct hdr), getHeaderServer(packet)->dataSize);
      flag = (rand()% 50) + 1;
      bool dropped = false;
      if (errors == 2 && flag == 4) {
        cout << "Packet " << getHeaderServer(packet)->seq << " dropped" << endl;
        dropped = true;
      }
      else if (errors == 3) {
        for (auto i = packetsToDrop.rbegin(); i != packetsToDrop.rend(); ++i) {
          if (*i == getHeaderServer(packet)->seq) {
            cout << "Packet " << getHeaderServer(packet)->seq << " dropped" << endl;
            packetsToDrop.pop_back();
            dropped = true;
            break;
          }
        }
      }
      if (checkSum == getHeaderServer(packet)->checkSum && !dropped) {
        getHeaderServer(packet)->ack = true;
        cout << "Checksum okay" << endl;
        slidingWindow.push_front(packet);

        flag = (rand()% 50) + 1;
        if (errors == 2 && flag == 4) {
          cout << "Ack " << getHeaderServer(packet)->seq << " lost" << endl;
          getHeaderServer(packet)->ack = false;
          slidingWindow.remove(packet);
          dropped = true;
        }
        else if (errors == 3) {
          for (auto i = acksToLose.rbegin(); i != acksToLose.rend(); ++i) {
            if (*i == getHeaderServer(packet)->seq) {
              cout << "Ack " << getHeaderServer(packet)->seq << " lost" << endl;
              acksToLose.pop_back();
              getHeaderServer(packet)->ack = false;
              slidingWindow.remove(packet);
              dropped = true;
              break;
            }
          }
        }
        if (!dropped) {
          /* Keep writing and sliding window while the beginning is equal to the correct packet sequence */
          bool loop = true;
          while (loop) {
            loop = false;
            for (auto i = slidingWindow.rbegin(); i != slidingWindow.rend(); ++i) {
              if (getHeaderServer(*i)->seq == windowStart && getHeaderServer(*i)->ack) {
                windowStart++;
                windowEnd++;
                if (windowEnd > seqEnd) {
                  windowEnd = 0;
                  wrappingMode = true;
                }
                if (windowStart > seqEnd) {
                  windowStart = 0;
                  wrappingMode = false;
                }
                fwrite(*i + sizeof(struct hdr), getHeaderServer(*i)->dataSize, 1, file);
                slidingWindow.remove(*i);
                loop = true;
                break;
              }
            }
          }
          cout << "Ack " << getHeaderServer(packet)->seq << " sent" << endl;
          send(clientSocket, packet, packetSize, 0);
          cout << printSlidingWindowServer(wrappingMode, windowStart, windowEnd, seqEnd) << endl;
        }
      }
      else if (!dropped){
        cout << "Checksum failed" << endl;
        /* If we are using SR and the checksum failed, we will send a negative ack back */
        if (protocol == 2) {
          cout << "Nack " << getHeaderServer(packet)->seq <<  " sent" << endl;
          send(clientSocket, packet, packetSize, 0);
        }
        cout << printSlidingWindowServer(wrappingMode, windowStart, windowEnd, seqEnd) << endl;
      }
    }
  }
  cout << "Last packet seq# received: " << lastSeq << endl;
  cout << "Number of original packets received: " << originalCounter << endl;
  cout << "Number of retransmitted packets received: " << retransmittedCounter << endl;

  fclose(file);
  close(clientSocket);
  return 0;
}

