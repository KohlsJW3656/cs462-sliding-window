#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string>
#include <bits/stdc++.h>
#include <cstdio>
#include <cstdlib>
#include <list>

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
  int checkSum;
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

/* Pass in char* array of bytes, get binary representation as string in bitStr */
void str2bs(char *bytes, size_t len, char *bitStr) {
  size_t i;
  char buffer[9] = "";
  for(i = 0; i < len; i++) {
    sprintf(buffer,
            "%c%c%c%c%c%c%c%c",
            (bytes[i] & 0x80) ? '1':'0',
            (bytes[i] & 0x40) ? '1':'0',
            (bytes[i] & 0x20) ? '1':'0',
            (bytes[i] & 0x10) ? '1':'0',
            (bytes[i] & 0x08) ? '1':'0',
            (bytes[i] & 0x04) ? '1':'0',
            (bytes[i] & 0x02) ? '1':'0',
            (bytes[i] & 0x01) ? '1':'0');
    strncat(bitStr, buffer, 8);
    buffer[0] = '\0';
  }
}

// function that creates the ones complement of a given string
// data the string to be ones complimented
string OnesCompelement(string data) {

  for (int i = 0; i < data.length(); i++) {

    if (data[i] == '0') {

      data[i] = '1';

    } else {

      data[i] = '0';

    }

  }

  return data;

}

// function that will return an integer value for a
// packet the string to be checksummed
// packetLen length of the data
// block_size size of the block
int createCheckSum(char *packet, unsigned int packetLen, int block_size) {
  // check if the block_size is divisable by dl if not add 0s in front of data
  if (packetLen % block_size != 0) {

    int pad_size = block_size - (packetLen % block_size);

    for (int i = 0; i < pad_size; i++) {

      *packet = '0' + *packet;

    }

  }

  // result of binary addition with carry
  string result = "";


  // first block stored in result
  for (int i = 0; i < block_size; i++) {

    result += packet[i];

  }

  // binary addition of the bock
  for (int i = block_size; i < packetLen; i += block_size) {

    // stores next block
    string next_block = "";

    for (int j = i; j < i + block_size; j++) {

      next_block += packet[j];

    }

    // stores the addition
    string additions = "";
    int sum = 0, carry = 0;

    for (int k = block_size - 1; k >= 0; k--) {

      sum += (next_block[k] - '0') + (result[k] - '0');
      carry = sum / 2;

      if (sum == 0) {

        additions = '0' + additions;
        sum = carry;

      } else if (sum == 1) {

        additions = '1' + additions;
        sum = carry;

      } else if (sum == 2) {

        additions = '0' + additions;
        sum = carry;

      } else {

        additions = '1' + additions;
        sum = carry;

      }

    }

    // after the addition check if carry is 1 then store the addition with carry result in final
    string final = "";

    if (carry == 1) {

      for (int m = additions.length() - 1; m >= 0; m--) {

        if (carry == 0) {

          final = additions[m] + final;

        } else if (((additions[m] - '0') + carry) == 0) {

          final = '0' + final;
          carry = 1;

        } else {

          final = '1' + final;
          carry = 0;

        }

      }

      result = final;

    } else {

      result = additions;

    }

  }

  string onesComp = OnesCompelement(result);
  int res = stoi(onesComp, 0, 2);

  return res;

}

struct hdr* getHeader(char* packet) {
  return (struct hdr*) packet;
}

void printPacket(char* packet, int packetSize) {
  cout << "Seq: " << getHeader(packet)->seq << endl;
  cout << "Checksum: " << getHeader(packet)->checkSum << endl;
  cout << "Ack: " << getHeader(packet)->ack << endl;
  cout << "Data Size: " << getHeader(packet)->dataSize << endl;

  for (int i = sizeof(struct hdr); i < packetSize; i++) {
    printf("%02X ", packet[i]);
  }
  cout << "\n";
}

void convertDataToBits(char* packet, unsigned int dataSize, char *bitStr2) {
  for (int i = sizeof(struct hdr); i < dataSize; i++) {
    char *bitStr = (char*)malloc(dataSize * sizeof(char));
    str2bs(&packet[i], 8, bitStr);
    strncat(bitStr2, bitStr, 8);
  }
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
//
//        cout << "Would you like random errors? 1 yes 2 no\n";
//        cin >> randInput;
//
//        if (randInput == 1) {
//
//            int randNum = (rand()%packetSize);
//
//            errorPacket = randNum;
//
//        } else {
//
//            cout << "Enter the packet to error: \n";
//            cin >> errorPacket;
//
//        }
//
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
    //return 1;
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

        //char *bitStr2 = (char*)malloc((dataSize + 1 ) * sizeof(char));
        //convertDataToBits(packet, dataSize, bitStr2);
        //int checkSum = createCheckSum(bitStr2, dataSize, 16);
        auto *packetHeader = (struct hdr *) packet;
        packetHeader->seq = packetSeqCounter;
        //->checkSum = checkSum;
        packetHeader->dataSize = dataSize + 1;
        packetHeader->ack = false;
        packetSeqCounter++;
      }
    }
    /* Send all packets in our slidingWindow */
    for (auto i = slidingWindow.rbegin(); i != slidingWindow.rend(); ++i) {
      /* Send to server */
      cout << "Packet " << getHeader(*i)->seq << " sent" << endl;
      printPacket(*i, getHeader(*i)->dataSize);
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
          printPacket(packet, packetSize);
        }
      }
      /* Clean up pointer */
      packet = nullptr;
      delete packet;
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
