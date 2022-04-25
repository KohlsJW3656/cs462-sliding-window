#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <list>
#include <iomanip>
#include <chrono>
#include <sys/epoll.h>
#include "boost/crc.hpp"

using namespace std;

//#define DEBUG
#define MAX_EVENTS 10

struct hdr {
  int seq;
  unsigned int dataSize;
  uint32_t checkSum;
  bool ack;
  bool sent;
  bool retransmitted;
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
  cout << endl;
}

string printSlidingWindow(bool wrappingMode, int windowStart, int windowEnd, int seqEnd) {
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

int sender(string ip, int port, int protocol, int packetSize, int timeoutInterval, int slidingWindowSize, int seqEnd, int errors) {
  char *filename = (char*)malloc(20 * sizeof(char));
  FILE *file;
  char* packet;
  list<char*> slidingWindow;
  list<std::chrono::system_clock> timeoutClocks;
  string userInput;
  int packetSeqCounter = 0;
  int retransmittedCounter = 0;
  int originalCounter = 0;
  int windowStart = 0;
  int windowEnd = slidingWindowSize - 1;
  unsigned int throughput = 0;
  bool wrappingMode = false;
  srand (time(NULL));
  int flag = 0;
  using clock = std::chrono::system_clock;
  //using ms = std::chrono::duration<double, std::milli>;
  using sec = std::chrono::duration<double>;
  list<int>packetsToCorrupt;
  int numPacketsToCorrupt;

  const auto startTime = clock::now();

  if (errors == 3) {
    cout << "Number of packets to corrupt: ";
    cin >> numPacketsToCorrupt;
    for (int i = 0; i < numPacketsToCorrupt; i++) {
      cout << "Sequence to corrupt: ";
      int corrupted;
      cin >> corrupted;
      packetsToCorrupt.push_front(corrupted);
    }
  }

  do {
    cout << "Please enter the file name: ";
    cin >> filename;
    file = fopen(filename, "rb");
    if (!file) {
      cout << "Invalid filename" << endl;
    }
  }
  while (!file);

  /* Create a socket */
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock == -1) {
    cout << "Failed to create socket" << endl;
    return 1;
  }

  /* Bind the ip address and port to a socket */
  sockaddr_in hint;
  hint.sin_family = AF_INET;
  hint.sin_port = htons(port);
  inet_pton(AF_INET, ip.c_str(), &hint.sin_addr);

  /* Connect to the server on the socket */
  int connectRes = connect(sock, (sockaddr*)&hint, sizeof(hint));
  if (connectRes == -1) {
    cout << "Failed to connect to server" << endl;
    return -1;
  }

  /* Create epoll file descriptor */
  struct epoll_event ev, events[MAX_EVENTS];
  int nfds, epollfd;
  epollfd = epoll_create1(0);
  if (epollfd == -1) {
    perror("epoll_create1");
    return -1;
  }

  ev.events = EPOLLIN;
  ev.data.fd = sock;
  if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sock, &ev) == -1) {
    perror("epoll_ctl: listen_sock");
    return -1;
  }

  /* While we haven't reached the end of the file */
  while(!feof(file)) {
    /* While our sliding window hasn't been filled up */
    while (slidingWindow.size() < slidingWindowSize) {
      packet = (char *) malloc(packetSize + 1);
      unsigned int dataSize = fread(packet + sizeof(struct hdr), 1, packetSize - sizeof(struct hdr), file);
      /* If reading successful, push packet to front and increase the sequence */
      if (dataSize) {
        throughput += dataSize + sizeof(struct hdr);
        slidingWindow.push_front(packet);
        uint32_t checkSum = GetCrc32(packet + sizeof(struct hdr), dataSize);
        auto *packetHeader = (struct hdr *) packet;
        packetHeader->seq = packetSeqCounter;
        packetHeader->dataSize = dataSize;
        packetHeader->checkSum = checkSum;
        packetHeader->ack = false;
        packetHeader->sent = false;
        packetHeader->retransmitted = false;
        packetSeqCounter++;
        if (packetSeqCounter == seqEnd + 1) {
          packetSeqCounter = 0;
        }
      }
      else {
        break;
      }
    }
    /* Send all packets in our slidingWindow */
    for (auto i = slidingWindow.rbegin(); i != slidingWindow.rend(); ++i) {
      /* If we haven't sent the packet and haven't acked */
      if (!getHeader(*i)->sent) {
        bool corrupted = false;
        cout << "Packet " << getHeader(*i)->seq << " sent";
        getHeader(*i)->sent = true;
        originalCounter++;
        #ifdef DEBUG
          printPacket(*i, getHeader(*i)->dataSize);
        #endif
        flag = (rand()% 50) + 1;
        if (errors == 2 && flag == 4) {
          cout << " with damaged checksum " << endl;
          uint32_t tempSum = getHeader(*i)->checkSum;
          getHeader(*i)->checkSum = GetCrc32(packet, getHeader(*i)->dataSize + sizeof(struct hdr));
          send(sock, *i, getHeader(*i)->dataSize + sizeof(struct hdr), 0);
          getHeader(*i)->checkSum = tempSum;
          corrupted = true;
        }
        else if (errors == 3) {
          for (auto j = packetsToCorrupt.rbegin(); j != packetsToCorrupt.rend(); ++j) {
            if (*j == getHeader(*i)->seq) {
              cout << " with damaged checksum " << endl;
              uint32_t tempSum = getHeader(*i)->checkSum;
              getHeader(*i)->checkSum = GetCrc32(packet, getHeader(*i)->dataSize + sizeof(struct hdr));
              send(sock, *i, getHeader(*i)->dataSize + sizeof(struct hdr), 0);
              getHeader(*i)->checkSum = tempSum;
              packetsToCorrupt.remove(*j);
              corrupted = true;
              break;
            }
          }
        }
        if (!corrupted) {
          cout << endl;
          send(sock, *i, getHeader(*i)->dataSize + sizeof(struct hdr), 0);
        }
      }
    }
    /* Responses */
    while (!slidingWindow.empty()) {
      nfds = epoll_wait(epollfd, events, MAX_EVENTS, timeoutInterval);
      /* Check for timeout */
      if (nfds == 0 && !slidingWindow.empty()) {
        /* If GBN, resend whole window */
        if (protocol == 1) {
          for (auto j = slidingWindow.rbegin(); j != slidingWindow.rend(); ++j) {
            cout << "Packet " << getHeader(*j)->seq << " *****Timed Out *****" << endl;
            cout << "Packet " << getHeader(*j)->seq << " Re-transmitted." << endl;
            getHeader(*j)->retransmitted = true;
            retransmittedCounter++;
            send(sock, *j, getHeader(*j)->dataSize + sizeof(struct hdr), 0);
          }
        }
        else {
          cout << "Packet " << getHeader(slidingWindow.back())->seq << " *****Timed Out *****" << endl;
          cout << "Packet " << getHeader(slidingWindow.back())->seq << " Re-transmitted." << endl;
          getHeader(slidingWindow.back())->retransmitted = true;
          retransmittedCounter++;
          send(sock, slidingWindow.back(), getHeader(slidingWindow.back())->dataSize + sizeof(struct hdr), 0);
        }
      }
      else if (nfds == -1) {
        cout << "Error getting event" << endl;
        return -1;
      }
      else {
        int bytesReceived = 0;
        packet = (char *) malloc(packetSize + 1);
        bytesReceived = recv(sock, packet, packetSize, 0);
        /* Continue to read in bytes until we get the full packet */
        while (bytesReceived != packetSize) {
          bytesReceived += recv(sock, packet + bytesReceived, packetSize - bytesReceived, 0);
          if (bytesReceived == -1 || bytesReceived == 0) {
            break;
          }
        }
#ifdef DEBUG
        cout << bytesReceived << endl;
#endif
        if (bytesReceived == -1 || bytesReceived == 0) {
          break;
        } else {
          /* Remove packets from sliding window while the beginning is equal to the correct packet sequence */
          bool loop = true;
          while (loop) {
            loop = false;
            for (auto i = slidingWindow.rbegin(); i != slidingWindow.rend(); ++i) {
              /* If the packet that we received is in our window, update the ack status */
              if (getHeader(*i)->seq == getHeader(packet)->seq) {
                getHeader(*i)->ack = getHeader(packet)->ack;
              }
              /* If the start of the window has been acked, slide the window */
              if (getHeader(*i)->seq == windowStart && getHeader(*i)->ack) {
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
                slidingWindow.remove(*i);
                loop = true;
                break;
              }
            }
          }
          if (getHeader(packet)->ack) {
            cout << "Ack " << getHeader(packet)->seq << " received" << endl;
            cout << printSlidingWindow(wrappingMode, windowStart, windowEnd, seqEnd) << endl;
          }
          else if (protocol == 2 && !getHeader(packet)->ack) {
            /* Find the packet in the window and retransmit it */
            for (auto i = slidingWindow.rbegin(); i != slidingWindow.rend(); ++i) {
              if (getHeader(*i)->seq == getHeader(packet)->seq) {
                cout << "Nack " << getHeader(*i)->seq << " received" << endl;
                cout << "Packet " << getHeader(*i)->seq << " Re-transmitted." << endl;
                retransmittedCounter++;
                getHeader(*i)->retransmitted = true;
                send(sock, *i, getHeader(*i)->dataSize + sizeof(struct hdr), 0);
              }
            }
          }
        }
      }
    }
  }
  const sec duration = clock::now() - startTime;

  cout << "Session successfully terminated" << endl << endl;
  cout << "Number of original packets sent: " << originalCounter << endl;
  cout << "Number of retransmitted packets sent: " << retransmittedCounter << endl;
  cout << fixed << setprecision(3) << "Total elapsed time: " << duration.count() << endl;
  cout << "Total throughput (Mbps): " << (((double) throughput / 1000000) * 8) / duration.count() << endl;
  cout << "Effective throughput: " << ((double) throughput / 1000000) * 8 << "Mb" << endl;

  fclose(file);
  close(sock);
  close(epollfd);
  return 0;
}
