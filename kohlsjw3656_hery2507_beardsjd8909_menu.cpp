//
// Created by Jonas Kohls on 03/11/2022.
//

#include <iostream>
#include "kohlsjw3656_hery2507_beardsjd8909_client.h"
#include "kohlsjw3656_hery2507_beardsjd8909_server.h"
using namespace std;

int main() {
  string ip;
  int port, protocol, packetSize, timeoutType, timeoutInterval, multiFactor, slidingWindowSize, seqStart, seqEnd, userType;

  cout << "IP address: ";
  cin >> ip;
  cout << "Port: ";
  cin >> port;
  cout << "1. GBN" << endl << "2. SR\n";
  cout << "Please select an option: ";
  cin >> protocol;
  cout << "Size of packet: ";
  cin >> packetSize;
  cout << "1. Static Timeout interval" << endl << "2. Dynamic Timeout interval\n";
  cout << "Please select an option: ";
  cin >> timeoutType;
  /* Static */
  if (timeoutType == 1) {
    cout << "Timeout Interval: ";
    cin >> timeoutInterval;
  }
  /* Dynamic */
  else {
    cout << "Multiplication factor for timeout: ";
    cin >> multiFactor;
    /* (Ping ip 3 times divided by time) * multiFactor */
  }
  cout << "Size of Sliding Window: ";
  cin >> slidingWindowSize;
  cout << "Start sequence number: ";
  cin >> seqStart;
  cout << "End sequence number: ";
  cin >> seqEnd;

  cout << "1. Client" << endl << "2. Server\n";
  cout << "Please select an option: ";
  cin >> userType;

  /* Client */
  if (userType == 1) {
    /* Situational error prompts */
    client(ip, port, protocol, packetSize, timeoutType, timeoutInterval, multiFactor, slidingWindowSize, seqStart, seqEnd, userType);
  }
  /* Server */
  else {
    server(port, protocol, packetSize, timeoutType, timeoutInterval, multiFactor, slidingWindowSize, seqStart, seqEnd, userType);
  }
}
