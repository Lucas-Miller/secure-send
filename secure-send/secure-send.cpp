#include <iostream>
#include <thread>
#include <winsock2.h> // for sockets
#include <ws2tcpip.h> // for inet_pton()
#include <string>

#pragma comment(lib, "ws2_32.lib") // link with ws2_32.lib

void receiveMessages(SOCKET sock)
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
        std::cout << "Received message: " << buffer << std::endl;

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
    inet_pton(AF_INET, "192.168.50.41", &remoteAddr.sin_addr); // replace with the IP address of the remote computer

    // create threads for sending and receiving messages
    std::thread receiveThread(receiveMessages, sock);
    std::thread sendThread(sendMessages, sock, remoteAddr);

    // wait for the threads to finish
    receiveThread.join();
    sendThread.join();

    // close the socket and clean up Winsock
    closesocket(sock);
    WSACleanup();

    return 0;
}