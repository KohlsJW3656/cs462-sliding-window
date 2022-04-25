//
// Created by Jonas Kohls on 03/11/2022.
//

#ifndef CS462_SLIDING_WINDOW_KOHLSJW3656_HERY2507_BEARDSJD8909_SENDER_H
#define CS462_SLIDING_WINDOW_KOHLSJW3656_HERY2507_BEARDSJD8909_SENDER_H
using namespace std;

int sender(string ip, int port, int protocol, int packetSize, int timeoutInterval, int slidingWindowSize, int seqEnd, int errors);

#endif //CS462_SLIDING_WINDOW_KOHLSJW3656_HERY2507_BEARDSJD8909_SENDER_H
