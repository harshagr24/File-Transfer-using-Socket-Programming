#include <iostream>
#include <fstream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <direct.h>

#pragma comment(lib, "ws2_32.lib")

#define BUFFER_SIZE 4096
#define DEFAULT_PORT 5001

void createDirectory(const std::string& path) {
    _mkdir(path.c_str());
}

void startServer(int port = DEFAULT_PORT) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed: " << WSAGetLastError() << std::endl;
        return;
    }

    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(port);

    if (bind(listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed: " << WSAGetLastError() << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        return;
    }

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed: " << WSAGetLastError() << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        return;
    }

    std::cout << "Server listening on port " << port << " (connect via 127.0.0.1 or your local IP)" << std::endl;

    SOCKET clientSocket = accept(listenSocket, NULL, NULL);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Accept failed: " << WSAGetLastError() << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        return;
    }

    // Set timeout (5 seconds)
    DWORD timeout = 5000;
    setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

    char buffer[BUFFER_SIZE];
    int bytesReceived;

    // Receive filename
    bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE, 0);
    if (bytesReceived <= 0) {
        std::cerr << "Failed to receive filename" << std::endl;
        closesocket(clientSocket);
        closesocket(listenSocket);
        WSACleanup();
        return;
    }
    buffer[bytesReceived] = '\0';
    std::string fileName(buffer);

    send(clientSocket, "OK", 2, 0); // Acknowledge filename

    // Receive filesize
    bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE, 0);
    if (bytesReceived <= 0) {
        std::cerr << "Failed to receive filesize" << std::endl;
        closesocket(clientSocket);
        closesocket(listenSocket);
        WSACleanup();
        return;
    }
    buffer[bytesReceived] = '\0';
    long fileSize = atol(buffer);

    send(clientSocket, "OK", 2, 0); // Acknowledge filesize

    // Receive file data
    createDirectory("received_files");
    std::string filePath = "received_files\\" + fileName;
    std::ofstream outputFile(filePath, std::ios::binary);
    if (!outputFile.is_open()) {
        std::cerr << "Failed to create file: " << filePath << std::endl;
        closesocket(clientSocket);
        closesocket(listenSocket);
        WSACleanup();
        return;
    }

    long totalReceived = 0;
    while (totalReceived < fileSize) {
        bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        if (bytesReceived <= 0) break;
        outputFile.write(buffer, bytesReceived);
        totalReceived += bytesReceived;
        std::cout << "\rReceived: " << totalReceived << "/" << fileSize << " bytes (" 
                  << (totalReceived * 100 / fileSize) << "%)" << std::flush;
    }

    outputFile.close();
    std::cout << "\nFile received successfully: " << filePath << std::endl;

    closesocket(clientSocket);
    closesocket(listenSocket);
    WSACleanup();
}

int main() {
    startServer();
    return 0;
}