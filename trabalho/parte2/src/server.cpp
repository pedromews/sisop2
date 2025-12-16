#include "../include/server.hpp"
#include "../include/udp_util.hpp"
#include "../include/discovery.hpp"
#include "../include/utils.hpp"
#include <iostream>
#include <arpa/inet.h>
#include <chrono>

using namespace std;

Server::Server(uint16_t port, uint32_t id, std::vector<Peer> peers, uint32_t registration_balance)
    : sock_(create_udp_server_socket(port)), id_(id), peers_(peers), state_(registration_balance),
      replica_manager_(id, peers, sock_, state_), election_(id, peers, sock_) {
    int yes = 1;
    setsockopt(sock_, SOL_SOCKET, SO_BROADCAST, &yes, sizeof(yes));
    cout << timestamp_now()
        << " num_transactions 0 total_transferred 0 total_balance 0" << endl;
}

void Server::run() {
    start_interface();

    // Bully Algorithm: Start election immediately upon recovery/startup
    cout << timestamp_now() << " [Server] Starting up. Initiating election." << endl;
    broadcast_presence();
    if (election_.start_election()) become_primary();

    auto last_hb_sent = std::chrono::steady_clock::now();
    auto last_presence_sent = std::chrono::steady_clock::now();

    while (true) {
        // Periodic tasks
        auto now = std::chrono::steady_clock::now();
        
        if (replica_manager_.is_primary()) {
            // Send heartbeats every 1s
            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_hb_sent).count() > 1000) {
                replica_manager_.send_heartbeats();
                last_hb_sent = now;
            }
        } else {
            // Check for leader failure
            if (replica_manager_.check_heartbeat_timeout() && !election_.is_in_progress()) {
                cout << timestamp_now() << " Leader timeout. Starting election." << endl;
                if (election_.start_election()) become_primary();
            }
        }

        // Broadcast presence periodically (every 3s) to ensure network convergence
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_presence_sent).count() > 3000) {
            broadcast_presence();
            last_presence_sent = now;
        }
        
        if (election_.check_timeout()) {
            if (!election_.higher_responded()) {
                cout << timestamp_now() << " Election timed out (no higher response). I am Coordinator." << endl;
                become_primary();
            } else {
                cout << timestamp_now() << " Election timed out (higher node failed). Restarting." << endl;
                if (election_.start_election()) become_primary();
            }
        }

        sockaddr_in src{};
        packet_t p{};
        // Use timeout (100ms) to allow periodic checks
        ssize_t n = udp_receive_packet(sock_, &p, sizeof(p), &src, 0.1);
        
        if (n <= 0) continue;

        packet_to_host(p);

        process_packet(p, src);
    }

    interface_thread_.join();
}

void Server::handle_exit(const sockaddr_in& src) {
    uint32_t client_ip = ntohl(src.sin_addr.s_addr);
    string client_ip_str = sockaddr_to_ipstr(src);

    try {
        state_.remove_client(client_ip);
        cout << timestamp_now() << " client " << client_ip_str << " has exited." << endl;
        
        auto stats = state_.get_stats();
        // cout << "Updated stats - num_transactions: " << stats.num_transactions
        //      << ", total_transferred: " << stats.total_transferred
        //      << ", total_balance: " << stats.total_balance << endl;
    } catch (const std::exception& e) {
        //cerr << timestamp_now() << " Error handling exit for client " << client_ip_str 
        //     << ": " << e.what() << endl;
    }

    // Send EXIT_ACK to client
    packet_t ack{};
    ack.type = PKT_EXIT_ACK;
    ack.seqn = 0;
    packet_to_network(ack);
    udp_send(sock_, &ack, sizeof(ack), &src);
}

void Server::process_packet(packet_t p, sockaddr_in src) {
    switch (p.type) {
        case PKT_DESC:
            if (replica_manager_.is_primary()) {
                handle_discovery_request(sock_, state_, src);
            }
            break;
        case PKT_REQ:
            if (replica_manager_.is_primary()) {
                cout << timestamp_now() << " [Server] Received REQ seqn " << p.seqn << " from " << sockaddr_to_ipstr(src) << ". Processing." << endl;
                thread worker(&Server::handle_request, this, p, src);
                worker.detach();
            } else {
                cout << timestamp_now() << " [Server] Received REQ seqn " << p.seqn << " from " << sockaddr_to_ipstr(src) << " but I am BACKUP. Ignoring." << endl;
            }
            break;
        case PKT_EXIT:
            handle_exit(src);
            break;
        case PKT_HEARTBEAT:
            // cout << timestamp_now() << " [Server] Received HEARTBEAT from " << p.body.elect.id << endl;
            replica_manager_.on_heartbeat_received(p.body.elect.id);
            
            // Conflict resolution: If I am primary but receive heartbeat from higher ID, step down.
            if (replica_manager_.is_primary() && p.body.elect.id > id_) {
                cout << timestamp_now() << " [Server] Detected higher ID " << p.body.elect.id << " acting as leader. Stepping down." << endl;
                packet_t coord_pkt{};
                coord_pkt.body.coord.id = p.body.elect.id;
                handle_coordinator(coord_pkt, src);
            }
            
            if (p.body.elect.id < id_ && !replica_manager_.is_primary() && !election_.is_in_progress()) {
                cout << timestamp_now() << " [Server] Detected leader " << p.body.elect.id << " with lower ID. Starting election." << endl;
                if (election_.start_election()) become_primary();
            }
            break;
        case PKT_ELECTION:
            cout << timestamp_now() << " [Server] Received ELECTION from " << p.body.elect.id << endl;
            if (replica_manager_.is_primary()) {
                cout << timestamp_now() << " [Server] Received ELECTION while Primary. Re-announcing victory." << endl;
                election_.announce_victory();
            } else if (election_.handle_election(p, src)) {
                cout << timestamp_now() << " [Server] Election triggered by peer. Starting my own election." << endl;
                if (election_.start_election()) become_primary();
            }
            break;
        case PKT_ELECTION_ACK:
            cout << timestamp_now() << " [Server] Received ELECTION_ACK from " << p.body.elect.id << endl;
            election_.handle_election_ack(p, src);
            break;
        case PKT_COORDINATOR:
            cout << timestamp_now() << " [Server] Received COORDINATOR from " << p.body.coord.id << endl;
            handle_coordinator(p, src);
            break;
        case PKT_STATE_UPDATE:
            if (!replica_manager_.is_primary()) {
                cout << timestamp_now() << " [Server] Received STATE_UPDATE seqn " << p.seqn << " from " << p.body.repl.src_addr << endl;
                replica_manager_.handle_replication(p);
            } else {
                cout << timestamp_now() << " [Server] Received STATE_UPDATE but I am PRIMARY. Ignoring." << endl;
            }
            break;
        case PKT_SNAPSHOT_REQ:
            if (replica_manager_.is_primary()) {
                cout << timestamp_now() << " [Server] Received SNAPSHOT_REQ from " << sockaddr_to_ipstr(src) << endl;
                replica_manager_.handle_snapshot_req(src);
            }
            break;
        case PKT_SNAPSHOT_DATA:
            cout << timestamp_now() << " [Server] Received SNAPSHOT_DATA." << endl;
            replica_manager_.handle_snapshot_data(p);
            break;
        case PKT_SNAPSHOT_STATS:
            cout << timestamp_now() << " [Server] Received SNAPSHOT_STATS." << endl;
            replica_manager_.handle_snapshot_stats(p);
            break;
        case PKT_SERVER_DESC: {
            if (p.body.sdesc.id == id_) break;
            cout << timestamp_now() << " [Server] Discovered peer " << p.body.sdesc.id << endl;
            Peer peer{p.body.sdesc.id, src};
            peer.addr.sin_port = htons(p.body.sdesc.port);
            replica_manager_.add_peer(peer);
            election_.add_peer(peer);
            
            // Send ACK
            packet_t ack{};
            ack.type = PKT_SERVER_DESC_ACK;
            ack.body.sdesc.id = id_;
            ack.body.sdesc.port = ntohs(src.sin_port); // Assuming symmetric port, or use configured port
            // Better to use the port we are listening on, which we know
            sockaddr_in my_addr{}; socklen_t len = sizeof(my_addr);
            getsockname(sock_, (sockaddr*)&my_addr, &len);
            ack.body.sdesc.port = ntohs(my_addr.sin_port);
            packet_to_network(ack);
            udp_send(sock_, &ack, sizeof(ack), &peer.addr);
            
            // If I am primary, send a heartbeat immediately so they know I am leader
            if (replica_manager_.is_primary()) {
                packet_t hb{};
                hb.type = PKT_HEARTBEAT;
                hb.body.elect.id = id_;
                packet_to_network(hb);
                udp_send(sock_, &hb, sizeof(hb), &peer.addr);
            }
            break;
        }
        case PKT_SERVER_DESC_ACK: {
            Peer peer{p.body.sdesc.id, src};
            peer.addr.sin_port = htons(p.body.sdesc.port);
            replica_manager_.add_peer(peer);
            election_.add_peer(peer);
            
            if (replica_manager_.is_primary()) {
                packet_t hb{};
                hb.type = PKT_HEARTBEAT;
                hb.body.elect.id = id_;
                packet_to_network(hb);
                udp_send(sock_, &hb, sizeof(hb), &peer.addr);
            }
            break;
        }
        default:
            cout << timestamp_now() << " [Server] Received unknown packet type " << p.type << endl;
            break;
    }
}

void Server::start_interface() {
    interface_thread_ = thread([this]() {
        while (true) {
            unique_lock<mutex> lock(print_mtx_);
            print_cv_.wait(lock, [this]() { return !print_queue_.empty(); });

            while (!print_queue_.empty()) {
                auto info = print_queue_.front();
                print_queue_.pop();
                lock.unlock();

                if (info.status != "OK" && info.status != "DUP"){
                    cout << timestamp_now() << " " << info.status << endl;
                    continue;
                }

                string status_str = (info.status != "OK")
                                    ? (string(" ") + info.status + "!")
                                    : string();

                cout << timestamp_now()
                     << " client " << info.client_ip
                     << status_str
                     << " id_req " << info.seqn
                     << " dest " << info.dest_ip
                     << " value " << info.value
                     << " num_transactions " << info.stats.num_transactions
                     << " total_transferred " << info.stats.total_transferred
                     << " total_balance " << info.stats.total_balance << endl;

                lock.lock();
            }
        }
    });
}

void Server::handle_request(packet_t p, sockaddr_in src) {
    if (!replica_manager_.is_primary()) {
        cout << "replica manager not primary" << endl;
        return;
    }

    uint32_t src_ip = ntohl(src.sin_addr.s_addr);
    uint16_t src_port = ntohs(src.sin_port);
    auto [ack_seq, balance, error] = state_.process_req(src_ip, src_port, p.seqn, p.body.req.dest_addr, p.body.req.value);

    // Replication
    if (error == NO_ERROR) {
        replica_manager_.replicate_request(p, src_ip, src_port);
    }

    //cerr << "error = " << error << endl;

    packet_t ack{};
    ack.type = PKT_REQ_ACK;
    ack.seqn = ack_seq;
    ack.body.ack.seqn = ack_seq;
    ack.body.ack.new_balance = balance;
    ack.body.ack.error = error;
    packet_to_network(ack);
    udp_send(sock_, &ack, sizeof(ack), &src);

    string client_ip = sockaddr_to_ipstr(src);
    in_addr dest_in; dest_in.s_addr = htonl(p.body.req.dest_addr);
    string dest_ip = inet_ntoa(dest_in);
    string status;

    switch (error) {
        case ERR_CLIENT_NOT_FOUND:
            status = "CLIENT NOT FOUND";
            break;
        case ERR_DUPLICATE_REQ:
            status = "DUP";
            break;
        case ERR_MISSING_PREV_REQ:
            status = "MISSING";
            break;
        case ERR_INSUFFICIENT_FUNDS:
            status = "NO FUNDS";
            break;
        default:
            status = "OK";
            break;
    }

    //cerr << "status = " << status << endl;

    auto stats = state_.get_stats();
    {
        lock_guard<mutex> lock(print_mtx_);
        print_queue_.push({client_ip, p.seqn, dest_ip, p.body.req.value, status, stats});
    }
    print_cv_.notify_one();
}

void Server::handle_coordinator(packet_t p, sockaddr_in src) {
    uint32_t new_leader = p.body.coord.id;
    replica_manager_.set_leader_id(new_leader);
    replica_manager_.set_primary(new_leader == id_);
    election_.stop();
    if (replica_manager_.is_primary()) notify_clients_leader_change();
    
    // Treat coordinator msg as heartbeat to reset timer
    replica_manager_.on_heartbeat_received(new_leader); 
    
    if (!replica_manager_.is_primary()) {
        if (state_.get_stats().num_transactions > 0)
            replica_manager_.send_snapshot_to_leader();
        else
            replica_manager_.request_snapshot();
    }
    
    cout << timestamp_now() << " new coordinator: " << new_leader << (replica_manager_.is_primary() ? " (me)" : "") << endl;
}

void Server::become_primary() {
    replica_manager_.set_primary(true);
    replica_manager_.set_leader_id(id_);
    election_.announce_victory();
    cout << timestamp_now() << " this server is the new coordinator (ID: " << id_ << ")" << endl;
    notify_clients_leader_change();
}

void Server::notify_clients_leader_change() {
    auto clients = state_.get_all_clients();
    packet_t p{};
    p.type = PKT_LEADER_CHANGE;
    p.seqn = 0;
    packet_to_network(p);
    
    for (const auto& c : clients) {
        if (c.port == 0) continue;
        sockaddr_in dest{};
        dest.sin_family = AF_INET;
        dest.sin_addr.s_addr = htonl(c.address);
        dest.sin_port = htons(c.port);
        udp_send(sock_, &p, sizeof(p), &dest);
    }
}

void Server::broadcast_presence() {
    packet_t p{};
    p.type = PKT_SERVER_DESC;
    p.body.sdesc.id = id_;
    sockaddr_in my_addr{}; socklen_t len = sizeof(my_addr);
    getsockname(sock_, (sockaddr*)&my_addr, &len);
    p.body.sdesc.port = ntohs(my_addr.sin_port);
    packet_to_network(p);

    sockaddr_in bcast{};
    bcast.sin_family = AF_INET;
    bcast.sin_port = my_addr.sin_port;
    bcast.sin_addr.s_addr = INADDR_BROADCAST;
    
    udp_send(sock_, &p, sizeof(p), &bcast);
}
