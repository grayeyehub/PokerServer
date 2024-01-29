#pragma once

#include "plotocol.h"
#include "TableStruct.h"
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "MSWSock.lib")

void display_error(const char* msg, int err_no)
{
    WCHAR* lpMsgBuf;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, err_no, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);
    cout << msg;
    wcout << lpMsgBuf << endl;
    LocalFree(lpMsgBuf);
}

void send_packet(int Client_id, void* pPacket)
{
    int p_size = reinterpret_cast<unsigned char*>(pPacket)[0];
    int p_type = reinterpret_cast<unsigned char*>(pPacket)[1];
    cout << "To client [ " << Client_id << "] : ";//디버깅용 (나중에 삭제해야함)
    cout << "Packet [" << p_type << "]\n";

    EX_OVER* s_over = new EX_OVER; //로컬 변수로 절때 하지말것 send계속 사용할것이니
    s_over->m_op = OP_SEND;
    memset(&s_over->m_over, 0, sizeof(s_over->m_over));
    memcpy(s_over->m_packetbuf, pPacket, p_size);
    s_over->m_wsabuf[0].buf = reinterpret_cast<CHAR*>(s_over->m_packetbuf);
    s_over->m_wsabuf[0].len = p_size;
    auto ret = WSASend(players[Client_id].m_socket, s_over->m_wsabuf, 1, NULL, 0, &s_over->m_over, 0);
    if (0 != ret) {
        auto err_no = WSAGetLastError();
        if (WSA_IO_PENDING != err_no)
            display_error("Error in SendPacket: ", err_no);
    }
}


void send_login_ok_packet(int Packet_id)
{
    S2CAckLoginOK PacketLoginOk;
    PacketLoginOk.id = Packet_id;
    PacketLoginOk.size = sizeof(PacketLoginOk);
    PacketLoginOk.type = S2C_Ack_Login_OK;
    send_packet(Packet_id, &PacketLoginOk);
}

void do_recv(int Packet_id)
{
    players[Packet_id].m_recv_over.m_wsabuf[0].buf = reinterpret_cast<char*>(players[Packet_id].m_recv_over.m_packetbuf) + players[Packet_id].m_prev_size;
    players[Packet_id].m_recv_over.m_wsabuf[0].len = MAX_BUFFER - players[Packet_id].m_prev_size;
    memset(&players[Packet_id].m_recv_over.m_over, 0, sizeof(players[Packet_id].m_recv_over.m_over));
    DWORD r_flag = 0;
    auto ret = WSARecv(players[Packet_id].m_socket, players[Packet_id].m_recv_over.m_wsabuf, 1, NULL, &r_flag, &players[Packet_id].m_recv_over.m_over, NULL);

    if (0 != ret) {
        auto err_no = WSAGetLastError();
        if (WSA_IO_PENDING != err_no)
            display_error("Error in SendPacket: ", err_no);
    }
}

int get_new_player_id(SOCKET p_socket)
{
    for (int i = SERVER_ID + 1; i < MAX_USER; ++i) {
        lock_guard<mutex>gl{ players[i].m_slock }; // lock_guard는 자동으로 언락을 해준다.
        if (PL_ST_FREE == players[i].m_state) {
            players[i].m_state = PL_ST_CONNECTED;
            players[i].m_socket = p_socket;
            players[i].m_name[0] = 0;
            return i;
        }

    }
    return -1;
}

void send_add_player(int Client_id, int Pakcet_id)
{
    S2CAddPlayer PacketAddPlayer;
    PacketAddPlayer.id = Pakcet_id;
    PacketAddPlayer.size = sizeof(PacketAddPlayer);
    PacketAddPlayer.type = S2C_Add_Player;
    send_packet(Client_id, &PacketAddPlayer);
}
void send_ack_draw_card(int Client_id, int Pakcet_id,int CNum,int CType)
{
    S2CAckDrawCard PacketDrawCard;
    PacketDrawCard.id = Pakcet_id;
    PacketDrawCard.size = sizeof(Pakcet_id);
    PacketDrawCard.type = S2C_Ack_DrawCard;
    PacketDrawCard.CardNum = CNum;
    PacketDrawCard.CardNum = CType;

    send_packet(Client_id, &PacketDrawCard);
}

void send_move_packet(int c_id, int p_id)
{
    s2c_move_player p;
    p.id = p_id;
    p.size = sizeof(p);
    p.type = S2C_MOVE_PLAYER;
    p.x = players[p_id].x;
    p.y = players[p_id].y;
    send_packet(c_id, &p);
}

void proccess_packet(int p_id, unsigned char* p_buf) {

    switch (p_buf[1])
    {
    case C2S_LOGIN: {
        c2s_login* packet = reinterpret_cast<c2s_login*>(p_buf);
        lock_guard<mutex>gl2{ players[p_id].m_slock };
        strcpy_s(players[p_id].m_name, packet->name);
        send_login_ok_packet(p_id);
        players[p_id].m_state = PL_ST_INGAME;

        for (auto& pl : players) {
            if (p_id != pl.id)
                lock_guard<mutex>gl{ pl.m_slock };
            if (PL_ST_INGAME == pl.m_state) {
                send_add_player(pl.id, p_id);
                send_add_player(p_id, pl.id);
            }
        }
    }

                  break;
    case C2S_MOVE: {
        c2s_move* packet = reinterpret_cast<c2s_move*>(p_buf);
        do_move(p_id, packet->dr);
    }

                 break;
    default:
        cout << "Unknown Packet Type from Client[" << p_id << "] Packet Type [" << p_buf[1] << "]" << endl;
        while (true);
    }
}


void disconnect(int p_id)
{
    {
        lock_guard<mutex>gl{ players[p_id].m_slock };
        closesocket(players[p_id].m_socket);
        players[p_id].m_state = PL_ST_FREE;
    }
    for (auto& pl : players) {
        if (false == is_npc(pl.id)) {

            lock_guard<mutex>gl2{ pl.m_slock };
            if (PL_ST_INGAME == pl.m_state)
                send_remove_player(pl.id, p_id);
        }
    }
}


void worker(HANDLE h_iocp, SOCKET l_socket) {
    while (true) {
        DWORD num_bytes;
        ULONG_PTR ikey;
        WSAOVERLAPPED* over;

        BOOL ret = GetQueuedCompletionStatus(h_iocp, &num_bytes, &ikey, &over, INFINITE);


        int key = static_cast<int>(ikey);
        if (FALSE == ret) {
            if (SERVER_ID == key) {
                display_error("GQCS:", WSAGetLastError());
                exit(-1);
            }
            else {
                display_error("GQCS:", WSAGetLastError());
                disconnect(key);
            }

        }
        if ((key != SERVER_ID) && (num_bytes == 0)) {
            disconnect(key);
            continue;
        }

        EX_OVER* ex_over = reinterpret_cast<EX_OVER*> (over);

        switch (ex_over->m_op)
        {
        case OP_RECV: {
            // 패킷 조립 및 처리
            unsigned char* packet_ptr = ex_over->m_packetbuf;
            int num_data = num_bytes + players[key].m_prev_size;
            int packet_size = packet_ptr[0];

            while (num_data >= packet_size) {
                proccess_packet(key, packet_ptr);
                num_data -= packet_size;
                packet_ptr += packet_size;
                if (0 >= num_data) break;
                packet_size = packet_ptr[0];
            }
            players[key].m_prev_size = num_data;
            if (0 != num_data)
                memcpy(ex_over->m_packetbuf, packet_ptr, num_data);

            do_recv(key);
            break;
        }
        case OP_SEND:
            delete ex_over;
            break;
        case OP_ACCEPT: {
            int c_id = get_new_player_id(ex_over->m_csocket);
            if (-1 != c_id) {
                players[c_id].m_recv_over.m_op = OP_RECV;
                players[c_id].m_prev_size = 0;
                CreateIoCompletionPort(reinterpret_cast<HANDLE>(players[c_id].m_socket), h_iocp, c_id, 0);

                do_recv(c_id);
            }
            else {
                closesocket(players[c_id].m_socket);
            }

            //나중에 함수로
            memset(&ex_over->m_over, 0, sizeof(ex_over->m_over));
            SOCKET c_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
            ex_over->m_csocket = c_socket;
            AcceptEx(l_socket, c_socket, ex_over->m_packetbuf, 0, 32, 32, NULL, &ex_over->m_over);
        }
                      break;
        }
    }
}

void do_random_move(SESSION& npc) {

    unordered_set<int> old_yl;
    for (auto& obj : objects) {
        if (PL_ST_INGAME != obj.m_state)continue;
        if (true == is_npc(obj.id))continue;
        if (true == can_see(npc.id, obj.id))
            old_yl.insert(obj.id);
    }





    int x = npc.x;
    int y = npc.y;

    switch (rand() % 4) {
    case 0: if (x < WORLD_X_SIZE - 1)x++; break;
    case 1: if (x > 0)x--; break;
    case 2: if (y < WORLD_Y_SIZE)y++; break;
    case 3: if (y > 0)y--; break;
    }
    npc.x = x;
    npc.y = y;

    unordered_set<int> new_yl;
    for (auto& obj : objects) {
        if (PL_ST_INGAME != obj.m_state)continue;
        if (true == is_npc(obj.id))continue;
        if (true == can_see(npc.id, obj.id))
            new_yl.insert(obj.id);
    }


    for (auto pl : new_yl) {


    }


}

void do_ai() {
    using namespace chrono;

    for (;;) {
        auto start_t = chrono::system_clock::now();
        for (auto& npc : objects) {
            if (true == is_npc(npc.id)) {
                do_npc_random_move(npc);

            }
        }
        auto end_t = chrono::system_clock::now();
        auto ai_time = end_t - start_t;
        cout << "AI Exerc Time : " << duration_cast<milliseconds>(ai_time).count();
        if (end_t < strat_t + chrono::seconds(1)) {
            this_thread::sleep_for(start_t + chrono::seconds(1) - end_t);
        }
    }

}
void do_timer() {
    using namespace chrono;

    if (timer_queue_top().start_time <= system_clock::now()) {
        TIMER_EVENT ev = timer_queue_top();
        time_queue.pop();
        switch (ev.e_type) {
        case OP_RANDOM_MOVE:
            do_npc_random_move(objects[ev.object]);
            add_event(ev.object, OP_RNADOM_MOVE, 1000);
            break;
            case OP_

        }
    }


}
