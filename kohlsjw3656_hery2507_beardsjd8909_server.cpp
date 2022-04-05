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

// function that creates the ones complement of a given string
// data the string to be ones complimented
string Ones_compelement(string data) {

    for (int i = 0; i < data.length(); i++) {

        if (data[i] == '0') {

            data[i] = '1';

        } else {

            data[i] = '0';

        }

    }

    return data;

}

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

    return Ones_compelement(result);

}

// function to check if sender and reciver have the same checksum
// sender message of the sender
// recevier message of the reciver
// block_size integer size of the block of the data to checksummed
bool validateMessage (int sender, int recevier, int block_size, int error) {

  string s = bitset<16>(sender).to_string(); //convers sender to a string 0b indicates that it is in binary
  string r = bitset<16>(recevier).to_string(); //convers recevier to a string 0b indicates that it is in binary

    string sender_checksum = checkSum(s, block_size);

    if (error == 1) {

        sender_checksum = Ones_compelement(sender_checksum);

    }

    string recevier_checksum = checkSum(r + sender_checksum, block_size);

    if (count(recevier_checksum.begin(), recevier_checksum.end(), '0') == block_size) {

        return true;

    } else {

        return false;

    }

}

int server(int port, int protocol, int packetSize, int timeoutType, int timeoutInterval, int multiFactor, int slidingWindowSize, int seqEnd, int userType) {
  int listening = socket(AF_INET, SOCK_STREAM, 0);
  if (listening == -1) {
    cerr << "Failed to create socket!" << endl;
    return -1;
  }

  int errorInput;
  int randInput;
  int errorPacket;

  cout << "Would you like to have errors? 1 yes 2 no\n";
  cin >> errorInput;

  if (errorInput == 1) {

      cout << "Would you like random errors? 1 yes 2 no\n";
      cin >> randInput;

      if (randInput == 1) {

          int randNum = (rand()%packetSize);

          errorPacket = randNum;

      } else {

          cout << "Enter the packet to error: \n";
          cin >> errorPacket;

        }

  }

// Bind the ip address and port to a socket
  sockaddr_in hint;
  hint.sin_family = AF_INET;
  hint.sin_port = htons(port);
  inet_pton(AF_INET, "0.0.0.0", &hint.sin_addr);

  bind(listening, (sockaddr*)&hint, sizeof(hint));

  listen(listening, SOMAXCONN);

  // Wait for a connection
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

  // Close listening socket
  close(listening);

  // While loop: accept and echo message back to client
  char buf[4096];

  while (true) {
    memset(buf, 0, 4096);

    // Wait for client to send data
    int bytesReceived = recv(clientSocket, buf, 4096, 0);
    if (bytesReceived == -1) {
      cerr << "Error in recv(). Quitting" << endl;
      break;
    }

    if (bytesReceived == 0) {
      cout << "Client disconnected " << endl;
      break;
    }

    cout << string(buf, 0, bytesReceived) << endl;
    // Echo message back to client
    send(clientSocket, buf, bytesReceived + 1, 0);
  }

  // Close the socket
  close(clientSocket);
  return 0;
}
