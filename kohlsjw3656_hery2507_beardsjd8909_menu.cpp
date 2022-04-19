//
// Created by Jonas Kohls on 03/11/2022.
//

#include <iostream>
#include <chrono>
#include "kohlsjw3656_hery2507_beardsjd8909_sender.h"
#include "kohlsjw3656_hery2507_beardsjd8909_receiver.h"
using namespace std;

int main() {
  string ip;
  double timeoutInterval;
  int port, protocol, packetSize, timeoutType, multiFactor, slidingWindowSize, seqEnd, userType, errors;

  cout << "1. Sender" << endl << "2. Receiver\n";
  cout << "Please select an option: ";
  cin >> userType;

  /* Sender */
  if (userType == 1) {
    cout << "IP address: ";
    cin >> ip;
    cout << "Port: ";
    cin >> port;
    cout << "1. GBN" << endl << "2. SR" << endl;
    cout << "Please select an option: ";
    cin >> protocol;
    cout << "Size of Sliding Window: ";
    cin >> slidingWindowSize;
    cout << "End sequence number: ";
    cin >> seqEnd;
    while (slidingWindowSize > (seqEnd + 1) / 2) {
      cout << "Invalid max sequence" << endl;
      cout << "End sequence number: ";
      cin >> seqEnd;
    }
    cout << "Size of packet: ";
    cin >> packetSize;
    cout << "1. Static Timeout interval" << endl << "2. Dynamic Timeout interval" << endl;
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
      system(("ping " + ip + " -c 3").c_str());
      auto end = std::chrono::system_clock::now();
      std::chrono::duration<double> totalTime = end - start;
      double t = totalTime.count();
      timeoutInterval = (t/3) * multiFactor;
    }
    cout << "1. No Errors" << endl << "2. Random Errors" << endl << "3. User Specified" << endl;
    cout << "Please select an option: ";
    cin >> errors;
    sender(ip, port, protocol, packetSize, timeoutInterval, slidingWindowSize, seqEnd, errors);
  }
  /* Receiver */
  else {
    cout << "Port: ";
    cin >> port;
    cout << "1. GBN" << endl << "2. SR" << endl;
    cout << "Please select an option: ";
    cin >> protocol;
    if (protocol == 2) {
      cout << "Size of Sliding Window: ";
      cin >> slidingWindowSize;
    }
    else {
      slidingWindowSize = 1;
    }
    cout << "End sequence number: ";
    cin >> seqEnd;
    while (slidingWindowSize > (seqEnd + 1) / 2) {
      cout << "Invalid max sequence" << endl;
      cout << "End sequence number: ";
      cin >> seqEnd;
    }
    cout << "Size of packet: ";
    cin >> packetSize;
    cout << "1. No Errors" << endl << "2. Random Errors" << endl << "3. User Specified" << endl;
    cout << "Please select an option: ";
    cin >> errors;
    receiver(port, protocol, packetSize, slidingWindowSize, seqEnd, errors);
  }
}
