#include "RTSPClient.h"
#include <iostream>

int main(int argc, char* argv[])
{
    try
    {
        RTSPClient rtspClient;
        rtspClient.Start();
        //std::string request;
        //std::cin >> request;
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}