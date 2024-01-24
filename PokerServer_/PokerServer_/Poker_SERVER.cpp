#include "ClientServerPacket.h"
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "MSWSock.lib")


int main()
{

    for (int i = 0; i < MAX_USER + 1; ++i) {
        auto& pl = players[i];
        pl.id = i;
        pl.m_state = PL_ST_FREE;
    }

    WSADATA WSAData;
    WSAStartup(MAKEWORD(2, 2), &WSAData);
    HANDLE h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);

    wcout.imbue(locale("korean"));

    SOCKET listenSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    CreateIoCompletionPort(reinterpret_cast<HANDLE>(listenSocket), h_iocp, SERVER_ID, 0);
    SOCKADDR_IN serverAddr;
    memset(&serverAddr, 0, sizeof(SOCKADDR_IN));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
    ::bind(listenSocket, (struct sockaddr*)&serverAddr, sizeof(SOCKADDR_IN));
    listen(listenSocket, SOMAXCONN);

    EX_OVER accept_over;
    accept_over.m_op = OP_ACCEPT;
    memset(&accept_over.m_over, 0, sizeof(accept_over.m_over));
    SOCKET c_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    accept_over.m_csocket = c_socket;
    bool ret = AcceptEx(listenSocket, c_socket, accept_over.m_packetbuf, 0, 32, 32, NULL, &accept_over.m_over);

    if (FALSE == ret) {
        int err_num = WSAGetLastError();
        if(err_num != WSA_IO_PENDING)
        display_error(" ACEEPTEx Error", err_num);

    }

    //멀티쓰레드로 바꾼부분

    //thread ai_thread(do_ai);
    //ai_thread.join();

    thread time_thread(do_timer);
    time_thread.join();


    vector<thread> worker_threads;
    for (int i = 0; i < 4; ++i)
        worker_threads.emplace_back(worker, h_iocp,listenSocket);
    for (auto& th : worker_threads)
        th.join();

   
    closesocket(listenSocket);
    WSACleanup();
}