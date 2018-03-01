#include "asio.hpp"
#include "RTPClient.h"
using asio::ip::tcp;

class RTSPClient
{
public:
    RTSPClient();
    void Start();

private:
    void SendRTSPMessage();
    void RecvRTSPPayload();
    void RecvRTSPMessage();
    void ParseRTSPMessageHeader();
    void RecvRTPData();
    asio::io_service io_service;
    tcp::socket s;
    asio::streambuf messageBuffer, SendBuffer;
    asio::streambuf messageBufferDebug;
    enum messageType{DESCRIBE = 0, SETUP, PLAY};
    messageType currentMessage;

    //string from rtsp message
    std::string retCode, contentType, contentLength, sessionID;

    RTPClient clientRTP;
};