//
// Created by Jonas Kohls on 03/11/2022.
//

#ifndef SENDER_H
#define SENDER_H
using namespace std;

int sender(string ip, int port, int protocol, int packetSize, int timeoutInterval, int slidingWindowSize, int seqEnd, int errors);

#endif //SENDER_H

