//
// Created by Jonas Kohls on 03/11/2022.
//

#define DEBUG

#include <bits/stdc++.h>
#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string>
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
    unsigned int packetSize;
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

// function that will return the checksum value of the given string
// data the string to be checksummed
// block_size integer size of the block of the data to checksummed
string checkSum(string data, int block_size) {

    // size of the data
    int dl = data.length();

    // check if the block_size is divisable by dl if not add 0s in front of data
    if (dl % block_size != 0) {

        int pad_size = block_size - (dl % block_size);

        for (int i = 0; i < pad_size; i++) {

            data = '0' + data;

        }

    }

    // result of binary addition with carry
    string result = "";


    // first block stored in result
    for (int i = 0; i < block_size; i++) {

        result += data[i];

    }

    // binary addition of the bock
    for (int i = block_size; i < dl; i += block_size) {

        // stores next block
        string next_block = "";

        for (int j = i; j < i + block_size; j++) {

            next_block += data[j];

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

    return result;

}

// function to check if sender and reciver have the same checksum
// sender message of the sender
// recevier message of the reciver
// block_size integer size of the block of the data to checksummed
bool validateMessage (int sender, int recevier, int block_size, int error) {

  string s = bitset<16>(sender).to_string(); //converts sender to a string 0b indicates that it is in binary
  string r = bitset<16>(recevier).to_string(); //converts receiver to a string 0b indicates that it is in binary

    string sender_checksum = checkSum(s, block_size);
    string receiver_checksum = checkSum(r + sender_checksum, block_size);

    if (count(receiver_checksum.begin(), receiver_checksum.end(), '1') == block_size) {

        return true;

    } else {

        return false;

    }

}

struct hdr* getHeaderServer(char* packet) {
  return (struct hdr*) packet;
}

void printPacketServer(char* packet, int packetSize) {
  cout << "Checksum: " << getHeaderServer(packet)->checkSum << endl;
  for (int i = 0; i < packetSize; i++) {
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

int server(int port, int protocol, int packetSize, int timeoutType, int timeoutInterval, int multiFactor, int slidingWindowSize, int seqEnd, int userType) {
  list<char*> slidingWindow;
  char* packet;
  int listening = socket(AF_INET, SOCK_STREAM, 0);
  if (listening == -1) {
    cerr << "Failed to create socket!" << endl;
    return -1;
  }
  int windowStart = 0;
  int windowEnd = slidingWindowSize - 1;

  /* Bind the ip address and port to a socket */
  sockaddr_in hint;
  hint.sin_family = AF_INET;
  hint.sin_port = htons(port);
  inet_pton(AF_INET, "0.0.0.0", &hint.sin_addr);

  bind(listening, (sockaddr*)&hint, sizeof(hint));

  listen(listening, SOMAXCONN);

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
    while (slidingWindow.size() < slidingWindowSize) {
      // Wait for client to send data
      packet = (char*) malloc(packetSize + 1);
      int bytesReceived = recv(clientSocket, packet, packetSize, 0);
      /* If the packet fits within the sequence */
      if (getHeaderServer(packet)->seq >= windowStart && getHeaderServer(packet)->seq <= windowEnd) {
        cout << "Packet " << getHeaderServer(packet)->seq << " received" << endl;
        slidingWindow.push_front(packet);
        //TODO Checksum thing
        getHeaderServer(packet)->ack = true;
        if (getHeaderServer(packet)->ack) {
          cout << "Checksum okay" << endl;
          cout << "Packet " << getHeaderServer(packet)->seq << " sent" << endl;
          send(clientSocket, packet, bytesReceived + 1, 0);
          windowStart++;
          windowEnd++;
          cout << printSlidingWindowServer(windowStart, windowEnd) << endl;
          if (windowEnd > seqEnd) {
            windowEnd = 0;
          }
        }
        else {
          cout << "Checksum failed" << endl;
          send(clientSocket, packet, bytesReceived + 1, 0);
          cout << printSlidingWindowServer(windowStart, windowEnd) << endl;
        }
        //printPacketServer(packet, packetSize);
        packet = nullptr;
        delete packet;
        slidingWindow.pop_back();
      }
    }
  }

  // Close the socket
  close(clientSocket);
  return 0;
}
