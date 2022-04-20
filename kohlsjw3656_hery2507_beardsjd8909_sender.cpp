
//#define DEBUG

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
#include "boost/crc.hpp"

using namespace std;

struct hdr {
  int seq;
  uint32_t checkSum;
  bool ack;
  bool sent;
  bool retransmitted;
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

int sender(string ip, int port, int protocol, int packetSize, double timeoutInterval, int slidingWindowSize, int seqEnd, int errors) {
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
  int displayOutput;
  unsigned int throughput = 0;
  bool wrappingMode = false;
  srand (time(NULL));
  using clock = std::chrono::system_clock;
  //using ms = std::chrono::duration<double, std::milli>;
  using sec = std::chrono::duration<double>;

  const auto startTime = clock::now();

  do {
    cout << "Please enter the file name: ";
    cin >> filename;
    file = fopen(filename, "rb");
    if (!file) {
      cout << "Invalid filename\n";
    }
  }
  while (!file);
  cout << "1. Display Output" << endl << "2. No output (for large files)" << endl;
  cout << "Please select an option: ";
  cin >> displayOutput;

  /* Create a socket */
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock == -1) {
    cout << "Failed to create socket\n";
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
        throughput += dataSize + sizeof(struct hdr);
        slidingWindow.push_front(packet);
        uint32_t checkSum = GetCrc32(packet + sizeof(struct hdr), dataSize);
        auto *packetHeader = (struct hdr *) packet;
        packetHeader->seq = packetSeqCounter;
        packetHeader->checkSum = checkSum;
        packetHeader->dataSize = dataSize;
        packetHeader->ack = false;
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
        if (displayOutput == 1) {
          cout << "Packet " << getHeader(*i)->seq << " sent" << endl;
        }
        getHeader(*i)->sent = true;

        //TODO timeoutClocks.push_back(clock::now());
        originalCounter++;
        #ifdef DEBUG
          printPacket(*i, getHeader(*i)->dataSize);
        #endif
        send(sock, *i, getHeader(*i)->dataSize + sizeof(struct hdr), 0);
      }
      /* If the duration is greater than timeout interval, and we haven't acked the packet, resend */
//TODO      if (((double)(clock() - getHeader(*i)->sentTime) / (double) ((double) CLOCKS_PER_SEC / 1000)) > timeoutInterval && !getHeader(*i)->ack && getHeader(*i)->sent) {
if (false) {
        if (displayOutput == 1) {
          cout << "Packet " << getHeader(*i)->seq << " *****Timed Out *****" << endl;
        }
        /* if GBN, retransmit whole window */
        if (protocol == 1) {
          if (displayOutput == 1) {
            cout << "Retransmitting Window" << endl;
          }
          for (auto j = slidingWindow.rbegin(); j != slidingWindow.rend(); ++j) {
            if (displayOutput == 1) {
              cout << "Packet " << getHeader(*j)->seq << " Re-transmitted." << endl;
            }
            //TODO getHeader(*j)->sentTime = clock();
            getHeader(*j)->retransmitted = true;
            retransmittedCounter++;
            send(sock, *j, getHeader(*j)->dataSize + sizeof(struct hdr), 0);
          }
        }
        else {
          if (displayOutput == 1) {
            cout << "Packet " << getHeader(*i)->seq << " Re-transmitted." << endl;
          }
          //TODO getHeader(*i)->sentTime = clock();
          getHeader(*i)->retransmitted = true;
          retransmittedCounter++;
          send(sock, *i, getHeader(*i)->dataSize + sizeof(struct hdr), 0);
        }
      }
    }
    /* Responses */
    while (!slidingWindow.empty()) {
      /* Packet responses */
      int bytesReceived = 0;
      packet = (char*) malloc(packetSize + 1);
      bytesReceived = recv(sock, packet, packetSize, 0);
      /* Continue to read in bytes until we get the full packet */
      while (bytesReceived != packetSize) {
        bytesReceived += recv(sock, packet + bytesReceived, packetSize - bytesReceived, 0);
        if (bytesReceived == -1 || bytesReceived == 0) {
          break;
        }
      }

      if (bytesReceived == -1 || bytesReceived == 0) {
        break;
      }
      else {
        if (getHeader(packet)->ack) {
          if (displayOutput == 1) {
            cout << "Ack " << getHeader(packet)->seq << " received" << endl;
          }
          /* If this is the correct packet we were looking for */
          if (getHeader(packet)->seq == getHeader(slidingWindow.back())->seq) {
            slidingWindow.pop_back();
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
            if (displayOutput == 1) {
              cout << printSlidingWindow(wrappingMode, windowStart, windowEnd, seqEnd) << endl;
            }
          }
          /* Received ack out of order */
          else {
            if (displayOutput == 1) {
              cout << printSlidingWindow(wrappingMode, windowStart, windowEnd, seqEnd) << endl;
            }
            send(sock, slidingWindow.back(), getHeader(slidingWindow.back())->dataSize + sizeof(struct hdr), 0);
          }
        }
        else {
          for (auto j = slidingWindow.rbegin(); j != slidingWindow.rend(); ++j) {
            if (displayOutput == 1) {
              cout << "Packet " << getHeader(*j)->seq << " *****Timed Out *****" << endl;
              cout << "Packet " << getHeader(*j)->seq << " Re-transmitted." << endl;
            }
            //TODO getHeader(*j)->sentTime = clock();
            getHeader(*j)->retransmitted = true;
            retransmittedCounter++;
            send(sock, *j, getHeader(*j)->dataSize + sizeof(struct hdr), 0);
          }
        }
      }
    }
  }
  const sec duration = clock::now() - startTime;

  cout << "Session successfully terminated" << endl << endl;
  cout << "Number of original packets sent: " << originalCounter << endl;
  cout << "Number of retransmitted packets sent: " << retransmittedCounter << endl;
  cout << fixed << setprecision(3) << "Total elapsed time: " << duration.count() << "s" << endl;
  cout << "Total throughput (Mbps): " << throughput / duration.count() << "Mbps" << endl;
  cout << "Effective throughput: " << throughput << endl;

  fclose(file);
  close(sock);
  return 0;
}
