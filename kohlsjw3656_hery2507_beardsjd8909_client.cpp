#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string>
#include <bits/stdc++.h>

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
bool validateMessage (string sender, string recevier, int block_size) {

    string sender_checksum = checkSum(sender, block_size);
    string recevier_checksum = checkSum(recevier + sender_checksum, block_size);

    if (count(recevier_checksum.begin(), recevier_checksum.end(), '0') == block_size) {

        return true;

    } else {

        return false;

    }

}

int client(string ip, int port, int protocol, int packetSize, int timeoutType, int timeoutInterval, int multiFactor, int slidingWindowSize, int seqStart, int seqEnd, int userType) {
  //	Create a socket
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock == -1) {
    return 1;
  }

  sockaddr_in hint;
  hint.sin_family = AF_INET;
  hint.sin_port = htons(port);
  inet_pton(AF_INET, ip.c_str(), &hint.sin_addr);

  //	Connect to the server on the socket
  int connectRes = connect(sock, (sockaddr*)&hint, sizeof(hint));
  if (connectRes == -1) {
    return 1;
  }

  //	While loop:
  char buf[4096];
  string userInput;

  do {
    //		Enter lines of text
    cout << "> ";
    getline(cin, userInput);

    //		Send to server
    int sendRes = send(sock, userInput.c_str(), userInput.size() + 1, 0);
    if (sendRes == -1) {
      cout << "Could not send to server! Whoops!\r\n";
      continue;
    }

    //		Wait for response
    memset(buf, 0, 4096);
    int bytesReceived = recv(sock, buf, 4096, 0);
    if (bytesReceived == -1) {
      cout << "There was an error getting response from server\r\n";
    }
    else {
      //		Display response
      cout << "SERVER> " << string(buf, bytesReceived) << "\r\n";
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
