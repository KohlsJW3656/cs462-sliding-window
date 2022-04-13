//
// Created by Jonas Kohls on 03/11/2022.
//

#include <iostream>
#include <chrono>
#include "kohlsjw3656_hery2507_beardsjd8909_client.h"
#include "kohlsjw3656_hery2507_beardsjd8909_server.h"
using namespace std;

int main() {
  string ip;
  double timeoutInterval;
  int port, protocol, packetSize, timeoutType, multiFactor, slidingWindowSize, seqStart, seqEnd, userType;

  cout << "1. Client" << endl << "2. Server\n";
  cout << "Please select an option: ";
  cin >> userType;

  /* Client */
  if (userType == 1) {
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
      auto start = std::chrono::system_clock::now();
      system(("ping " + ip).c_str());
      auto end = std::chrono::system_clock::now();
      std::chrono::duration<double> totalTime = end - start;
      double t = totalTime.count();
      timeoutInterval = (t/3) * multiFactor;
    }
    cout << "Size of Sliding Window: ";
    cin >> slidingWindowSize;
    /* Situational error prompts */
    client(ip, port, protocol, packetSize, timeoutInterval, slidingWindowSize);
  }
  else {
    cout << "Port: ";
    cin >> port;
    cout << "1. GBN" << endl << "2. SR\n";
    cout << "Please select an option: ";
    cin >> protocol;
    cout << "Size of packet: ";
    cin >> packetSize;
    cout << "Size of Sliding Window: ";
    cin >> slidingWindowSize;
    cout << "End sequence number: ";
    cin >> seqEnd;
    server(port, protocol, packetSize, slidingWindowSize, seqEnd);
  }
}
