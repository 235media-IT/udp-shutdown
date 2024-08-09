#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#pragma comment(lib, "WS2_32.lib")

#include "../include/config.h"
#include <direct.h>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include "../resource.h"

#include <shellapi.h>
#include <windows.h>
#include <winuser.h >

// #pragma comment(lib, "ws2_32.lib") // Winsock Library

#define BUFLEN 512 // Max length of buffer

bool consoleVisible = true;

// Function to check if a specific registry key exists
bool IsVCredistInstalled(LPCWSTR keyPath) {
    HKEY hKey;
    LONG lRes = RegOpenKeyEx(HKEY_LOCAL_MACHINE, keyPath, 0, KEY_READ, &hKey);
    if (lRes == ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return true;
    }
    return false;
}

BOOL systemShutdown() {
    printf("[ INFO ] Shutting down Windows... ");
    WinExec("shutdown -f -s -t 0", SW_HIDE);
    Sleep(500); // Works without this but it's safer to use sleep
    // KillProcessTree("winlogon"); // Internal process killer you can use pskill64
    WinExec("pskill64 winlogon -t -nobanner /accepteula", SW_HIDE);
    exit(-10); // Planned Shutdown Code
}

BOOL systemReboot() {
    printf("[ INFO ] Rebooting Windows... ");
    int result = WinExec("shutdown -f -r -t 0", SW_HIDE);
    Sleep(500); // Works without this but it's safer to use sleep
    exit(-10); // Planned Shutdown Code
}

int main() {

    LPCWSTR vcRedist64 = L"SOFTWARE\\Microsoft\\VisualStudio\\14.0\\VC\\Runtimes\\x64";

    bool isVcRedistInstalled = IsVCredistInstalled(vcRedist64);

    puts("235Media udp-shutdown\n\n");

    if (isVcRedistInstalled) {
        std::wcout << L"[ INFO ] Visual C++ 2015-2022 Redistributable (x64) is installed." << std::endl;
    } else {
        std::wcout << L"[ ERROR ] Visual C++ 2015-2022 Redistributable (x64) is NOT installed. Install it to use this application! " << std::endl;
        while (1);
    }


    int window = ShowWindow(GetConsoleWindow(), SW_MINIMIZE);
    // configure config
    Config *config = new Config();
    std::string shutdown_cmd = config->shutdown_cmd;
    std::string reboot_cmd = config->reboot_cmd;
    int port = config->port;

    // setup socket
    SOCKET s;
    struct sockaddr_in server, si_other;
    int slen, recv_len;
    char buf[BUFLEN];
    WSADATA wsa;

    slen = sizeof(si_other);

    

    // Initialise winsock
    printf("[ INFO ] Initialising Winsock...\n");
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("[ ERR ] Failed. Error Code : %d", WSAGetLastError());
        exit(EXIT_FAILURE);
    }
    printf("[ OK ] Initialised.\n");

    // Create a socket
    if ((s = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET) {
        printf("[ ERR ] Could not create socket : %d", WSAGetLastError());
    }
    printf("[ OK ] Socket created.\n");

    // Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);
    printf("[ INFO ] Binding...\n");
    // Bind
    if (bind(s, (struct sockaddr *)&server, sizeof(server)) == SOCKET_ERROR) {
        printf("[ ERR ] Bind failed with error code : %d", WSAGetLastError());
        // exit(EXIT_FAILURE);
    }
    printf("[ OK ] Bind done\n");
    printf("[ INFO ] Listening for shutdown command: \"%s\" on port :%i\n", shutdown_cmd.c_str(), port);
    printf("[ INFO ] Listening for reboot command: \"%s\" on port :%i\n", reboot_cmd.c_str(), port);
    // keep listening for data
    while (1) {
        fflush(stdout);

        // clear the buffer by filling null, it might have previously received data
        memset(buf, '\0', BUFLEN);

        // try to receive some data, this is a blocking call
        if ((recv_len = recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *)&si_other, &slen)) == SOCKET_ERROR) {
            printf("[ ERR ] recvfrom() failed with error code : %d", WSAGetLastError());
            exit(EXIT_FAILURE);
        }

        // print details of the client/peer and the data received
        printf("[ INFO ] Received packet from %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
        printf("[ INFO ] Data: %s\n", buf);

        // exec shutdown if received data equals shutdown_cmd

        if (strcmp(buf, shutdown_cmd.c_str()) == 0) {
            systemShutdown();
        }
        if (strcmp(buf, reboot_cmd.c_str()) == 0) {
            systemReboot();
        }
    }
    closesocket(s);
    WSACleanup();
    // CloseHandle(hThread);
    return 0;
}

