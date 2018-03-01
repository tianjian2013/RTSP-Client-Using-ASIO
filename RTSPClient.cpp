#include "RTSPClient.h"

#include <iostream>
#include <thread>
#include <regex>

static char DescribeString[] = "DESCRIBE rtsp://192.168.1.102:554/stream1 RTSP/1.0\r\nCSeq: 1\r\nAccept: application/sdp\r\n\r\n";
static char SetupString[] = "SETUP rtsp://192.168.1.102:554/stream1/track1 RTSP/1.0\r\nCSeq: 2\r\nTransport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n\r\n";

RTSPClient::RTSPClient() : s(io_service),
                           currentMessage(DESCRIBE)
{

}

void RTSPClient::Start()
{
    tcp::resolver resolver(io_service);
    auto endpoint_iterator = resolver.resolve({ "192.168.1.102", "554" });
    asio::async_connect(s, endpoint_iterator,
        [this](std::error_code ec, tcp::resolver::iterator)
    {
        if (!ec)
        {
            asio::async_write(s, asio::buffer(DescribeString, sizeof(DescribeString)-1),
                [this](const asio::error_code& error, std::size_t bytes_transferred)
            {
                if (!error)
                {
                    RecvRTSPMessage();
                }
            });
        }
    });
    io_service.run();
}

void RTSPClient::ParseRTSPMessageHeader()
{
    contentLength.clear();

    std::istream is(&messageBuffer);
    std::string line;
    while (std::getline(is, line))
    {
        std::cout << line << std::endl;
        std::regex eRetCode("RTSP/1.0 (.*) (.*)\r");
        std::regex eContentType("Content-Type: (.*)\r");
        std::regex eContentLength("Content-Length: (.*)\r");
        std::regex eSessionID("Session: (.*);");
        std::smatch smRetCode, smContentType, smContentLength, smSessionID;

        std::regex_search(line, smRetCode, eRetCode);
        if (smRetCode.size() >= 3)
        {
            retCode = smRetCode[1].str();
        }
        
        std::regex_search(line, smContentType, eContentType);
        if (smContentType.size() >= 2)
        {
            contentType = smContentType[1].str();
        }

        std::regex_search(line, smContentLength, eContentLength);
        if (smContentLength.size() >= 2)
        {
            contentLength = smContentLength[1].str();
        }

        std::regex_search(line, smSessionID, eSessionID);
        if (smSessionID.size() >= 2)
        {
            sessionID = smSessionID[1].str();
        }

        if (line[0] == '\r')
            break;
    }
}

void RTSPClient::RecvRTSPPayload()
{

    if (contentLength.size() > 0)
    {
        // TODO : recv whole content.
        asio::async_read(s, messageBuffer, asio::transfer_at_least(50),
            [this](std::error_code ec, std::size_t length)
        {
            //parse sdp
            messageBuffer.consume(std::stoi(contentLength));
            currentMessage = SETUP;
            SendRTSPMessage();
        }

        );
    }
    else
    {
        if (currentMessage == SETUP)
        {
            currentMessage = PLAY;
            SendRTSPMessage();
        }
        else if (currentMessage == PLAY)
        {
            //start recv rtp data.
            RecvRTPData();
        }

        
    }

   

}

void RTSPClient::RecvRTPData()
{
    asio::async_read(s, messageBuffer, asio::transfer_at_least(15000),
        [this](std::error_code ec, std::size_t length)
    {
        asio::const_buffer RTPBuffer = messageBuffer.data();
        
        std::size_t bufferLength = asio::buffer_size(RTPBuffer);
        const unsigned char* buffer = asio::buffer_cast<const unsigned char*>(RTPBuffer);
        int i = 0;
        while (i < bufferLength)
        {
            if (buffer[i] == '$')
            {
                int RTPPacketLength = (buffer[i + 2] << 8) + buffer[i + 3];
                if (i + 4 + RTPPacketLength <= bufferLength)
                {
                    clientRTP.ProcessRTPPacket(buffer + i + 4, RTPPacketLength);
                    i += (4 + RTPPacketLength);
                }
                else
                {
                    break;
                }
            }
            else
            {
                i++;
            }
        }
        messageBuffer.consume(i);
        RecvRTPData();
    }

    );
}

void RTSPClient::RecvRTSPMessage()
{

    const std::string delim("\r\n\r\n");

    asio::async_read_until(s, messageBuffer, delim,
        [this](std::error_code ec, std::size_t length)
    {
        if (!ec)
        {
            ParseRTSPMessageHeader();
            RecvRTSPPayload();
        }
        else
        {
            s.close();
        }
    });
}

void RTSPClient::SendRTSPMessage()
{
    if (currentMessage == SETUP)
    {
        asio::async_write(s, asio::buffer(SetupString, sizeof(SetupString)-1),
            [this](const asio::error_code& error, std::size_t bytes_transferred)
        {
            if (!error)
            {
                RecvRTSPMessage();
            }
        });
    }
    else if (currentMessage == PLAY)
    {
        std::ostream os(&SendBuffer);
        os << "PLAY rtsp://192.168.1.102:554/stream1/ RTSP/1.0\r\n" \
           << "CSeq: 3\r\n" \
           << "Session: " << sessionID << "\r\n\r\n";
        asio::async_write(s, SendBuffer,
            [this](const asio::error_code& error, std::size_t bytes_transferred)
        {
            if (!error)
            {
                RecvRTSPMessage();
            }
        });

    }
}