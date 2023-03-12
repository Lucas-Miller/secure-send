#include <iostream>
#include <thread>
#include <winsock2.h> // for sockets
#include <ws2tcpip.h> // for inet_pton()
#include <string>

#pragma comment(lib, "ws2_32.lib") // link with ws2_32.lib

void receiveMessages(int sock);
void sendMessages(int sock, sockaddr_in remoteAddr);

void receiveMessages(SOCKET sock, const sockaddr_in* localIpAddr)
{
    char buffer[1024];
    sockaddr_in remoteAddr;
    int addrLen = sizeof(remoteAddr);

    while (true) {
        // receive a message from the remote computer
        int iResult = recvfrom(sock, buffer, sizeof(buffer), 0, (sockaddr*)&remoteAddr, &addrLen);
        if (iResult == SOCKET_ERROR) {
            std::cout << "recvfrom failed: " << WSAGetLastError() << std::endl;
            break;
        }
        buffer[iResult] = '\0';

        // check if the message is coming from a different machine
        char remoteIpAddr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &remoteAddr.sin_addr, remoteIpAddr, INET_ADDRSTRLEN);

        char localIpAddrStr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &localIpAddr->sin_addr, localIpAddrStr, INET_ADDRSTRLEN);

        if (strcmp(remoteIpAddr, localIpAddrStr) != 0) {
            std::cout << "/nReceived message from " << remoteIpAddr << ": " << buffer << std::endl;
        }

        // check if the message is 'Goodbye'
        if (std::string(buffer) == "Goodbye") {
            break;
        }
    }
}

void sendMessages(SOCKET sock, const sockaddr_in& remoteAddr)
{
    std::string message;

    do {
        // send a message to the remote computer
        std::cout << "Enter a message to send: ";
        std::getline(std::cin, message);
        int iResult = sendto(sock, message.c_str(), message.length(), 0, (sockaddr*)&remoteAddr, sizeof(remoteAddr));
        if (iResult == SOCKET_ERROR) {
            std::cout << "sendto failed: " << WSAGetLastError() << std::endl;
            break;
        }
    } while (message != "Goodbye");
}

int main()
{
    // initialize Winsock
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        std::cout << "WSAStartup failed: " << iResult << std::endl;
        return 1;
    }

    // create a socket for sending and receiving data
    // create a socket for sending and receiving data
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        std::cout << "socket creation failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    // set up the socket address structure
    sockaddr_in localAddr;
    localAddr.sin_family = AF_INET;
    localAddr.sin_port = htons(12345); // choose any port number
    localAddr.sin_addr.s_addr = INADDR_ANY;

    // allow broadcasting
    int broadcastEnabled = 1;
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char*)&broadcastEnabled, sizeof(broadcastEnabled));

    // bind the socket to the local address
    iResult = bind(sock, (sockaddr*)&localAddr, sizeof(localAddr));
    if (iResult == SOCKET_ERROR) {
        std::cout << "socket binding failed: " << WSAGetLastError() << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    // set up the remote address structure
    sockaddr_in remoteAddr;
    remoteAddr.sin_family = AF_INET;
    remoteAddr.sin_port = htons(12345); // choose the same port number as before

    // Get local IPv4 address
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == SOCKET_ERROR) {
        std::cout << "gethostname failed: " << WSAGetLastError() << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    addrinfo* addrInfo;
    addrinfo hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_DGRAM; // UDP
    hints.ai_protocol = IPPROTO_UDP;
    hints.ai_flags = AI_PASSIVE; // use local IP address

    iResult = getaddrinfo(hostname, NULL, &hints, &addrInfo);
    if (iResult != 0) {
        std::cout << "getaddrinfo failed: " << iResult << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    // use the first address returned by getaddrinfo()
    sockaddr_in* localIpAddr = (sockaddr_in*)addrInfo->ai_addr;

    // convert the local IP address to string
    char localIpStr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(localIpAddr->sin_addr), localIpStr, INET_ADDRSTRLEN);

    // print out the local IP address
    std::cout << "Local IP address: " << localIpStr << std::endl;

    // set the remote IP address to broadcast address
    remoteAddr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

    // create threads to send and receive messages simultaneously
    std::thread receiveThread([&sock, &localIpAddr]() {
        receiveMessages(sock, localIpAddr);
        });

    std::thread sendThread([&]() {
        sendMessages(sock, remoteAddr);
        });

    // wait for the threads to finish
    receiveThread.join();
    sendThread.join();

    // clean up
    closesocket(sock);
    WSACleanup();

    return 0;
}