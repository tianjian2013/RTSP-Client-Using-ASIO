#include "RTPClient.h"
#include <iostream>

void RTPClient::ProcessRTPPacket(const unsigned char * data, int length)
{
    unsigned short SN;
    SN = (data[2] << 8) + data[3];
    std::cout << SN << std::endl;
}