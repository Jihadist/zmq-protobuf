#include "msg.pb.h"
#include <chrono>
#include <functional>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <iostream>
#include <memory>
#include <random>
#include <thread>
#include <vector>
#include <zmq.hpp>
#define within(num) (int)((float)((num)*random()) / (RAND_MAX + 1.0))
//  This is our client task class.
//  It connects to the server, and then sends a request once per second
//  It collects responses as they arrive, and it prints them out. We will
//  run several client tasks in parallel, each with a different random ID.
//  Attention! -- this random work well only on linux.

class client_task {
public:
    client_task()
        : ctx_(1)
        , client_socket_(ctx_, ZMQ_DEALER)
    {
    }
    int counter_rep = 0;
    int counter_req = 0;
    void dump(zmq::socket_t& socket);
    void start()
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> uid(0, 10000);
        // generate random identity
        char identity[10] = {};
        sprintf(identity, "%04X-%04X", within(0x10000), within(0x10000));
        printf("%s\n", identity);

        client_socket_.set(zmq::sockopt::routing_id, identity);
        ;
        client_socket_.connect("ipc:///tmp/feeds/0");

        zmq::pollitem_t items[] = {
            { static_cast<void*>(client_socket_), 0, ZMQ_POLLIN, 0 }
        };
        int request_nbr = 0;
        try {
            while (true) {
                for (int i = 0; i < 100; ++i) {
                    // 10 milliseconds
                    zmq::poll(items, 1, 10);
                    if (items[0].revents & ZMQ_POLLIN) {
                        printf("\n%s ", identity);
                        dump(client_socket_);
                    }
                }
                int requests = within(5);
                for (int request = 0; request < requests; ++request) {
                    std::this_thread::sleep_for(std::chrono_literals::operator""ms(within(1000) + 1));
                    char request_string[16] = {};
                    sprintf(request_string, "request #%d", ++request_nbr);
                    zmq::message_t request_message(request_string, strlen(request_string));
                    std::cout << "Client send message " << request << " : " << request_message.str() << std::endl;
                    client_socket_.send(request_message, zmq::send_flags::none);
                    counter_req++;
                }
            }
        } catch (std::exception& e) {
        }
    }

private:
    zmq::context_t ctx_;
    zmq::socket_t client_socket_;
};

void client_task::dump(zmq::socket_t& socket)
{
    std::cout << "----------------------------------------" << std::endl;

    while (1) {
        //  Process all parts of the message
        zmq::message_t message;
        socket.recv(message);
        counter_rep++;
        std::cout << "Client worker received : " << message.str() << std::endl;
        //  Dump the message as text or binary
        size_t size = message.size();
        std::string data(static_cast<char*>(message.data()), size);

        bool is_text = true;

        size_t char_nbr;
        unsigned char byte;
        for (char_nbr = 0; char_nbr < size; char_nbr++) {
            byte = data[char_nbr];
            if (byte < 32 || byte > 127)
                is_text = false;
        }
        std::cout << "[" << std::setfill('0') << std::setw(3) << size << "]";

        std::string buffer;
        for (char_nbr = 0; char_nbr < size; char_nbr++) {
            if (is_text)
                std::cout << (char)data[char_nbr];
            else {
                buffer.push_back(data[char_nbr]);
            }
        }
        ZmqPBExampleWeather update;
        update.ParseFromString(buffer);
        std::cout << "\t\tzip: " << update.zipcode() << "\t\ttemperature: " << update.temperature() << "\t\trelative humidity: " << update.relhumidity() << std::endl;
        std::cout << std::endl;

        int more = socket.get(zmq::sockopt::rcvmore); //  Multipart detection
        if (!more)
            break; //  Last message part
    }
}

//  .split worker task
//  Each worker task works on one request at a time and sends a random number
//  of replies back, with random delays between replies:

class server_worker {
public:
    server_worker(zmq::context_t& ctx, int sock_type)
        : ctx_(ctx)
        , worker_(ctx_, sock_type)
    {
    }
    int counter_rep = 0;
    int counter_req = 0;
    void work()
    {
        worker_.connect("inproc://backend");
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> uid(0, 10000);
        try {
            while (true) {
                zmq::message_t identity;
                zmq::message_t msg;
                zmq::message_t copied_id;
                zmq::message_t copied_msg;
                auto id = worker_.recv(identity);
                auto ms = worker_.recv(msg);
                counter_rep++;
                std::cout << "Server worker: " << identity.str() << ", received : " << msg.str() << std::endl;

                int replies = within(5);
                for (int reply = 0; reply < replies; ++reply) {
                    std::this_thread::sleep_for(std::chrono_literals::operator""ms(within(1000) + 1));
                    copied_id.copy(identity);
                    using namespace google::protobuf::io;
                    std::string encoded_message;
                    ZmqPBExampleWeather update;
                    update.set_zipcode(uid(gen));
                    update.set_temperature(uid(gen));
                    update.set_relhumidity(uid(gen));

                    std::string serialized_update;
                    update.SerializeToString(&serialized_update);

                    zmq::message_t request_message(serialized_update);

                    std::cout << "Client send message with id {" << copied_id.str() << "} : "
                              << "\t\tzip: " << update.zipcode() << "\t\ttemperature: " << update.temperature() << "\t\trelative humidity: " << update.relhumidity() << std::endl;
                    worker_.send(copied_id, zmq::send_flags::sndmore);
                    worker_.send(request_message, zmq::send_flags::none);
                    counter_req++;
                }
            }
        } catch (std::exception& e) {
        }
    }

private:
    zmq::context_t& ctx_;
    zmq::socket_t worker_;
};

struct counters {
    int* counter_rep;
    int* counter_req;
};

//  .split server task
//  This is our server task.
//  It uses the multithreaded server model to deal requests out to a pool
//  of workers and route replies back to clients. One worker can handle
//  one request at a time but one client can talk to multiple workers at
//  once.

class server_task {
public:
    server_task()
        : ctx_(1)
        , frontend_(ctx_, ZMQ_ROUTER)
        , backend_(ctx_, ZMQ_DEALER)
    {
    }

    enum { kMaxThread = 5 };

    void run()
    {
        frontend_.bind("ipc:///tmp/feeds/0");
        backend_.bind("inproc://backend");

        std::vector<server_worker*> worker;
        std::vector<std::thread*> worker_thread;
        for (auto i = 0; i < kMaxThread; ++i) {
            auto new_worker = new server_worker(ctx_, ZMQ_DEALER);
            counters counter { &new_worker->counter_rep, &new_worker->counter_req };
            counters_.push_back(counter);
            worker.push_back(new_worker);

            worker_thread.push_back(new std::thread(std::bind(&server_worker::work, worker[i])));
            worker_thread[i]->detach();
        }

        try {
            zmq::proxy(frontend_, backend_);
        } catch (std::exception& e) {
        }

        for (auto i = 0; i < kMaxThread; ++i) {
            delete worker[i];
            delete worker_thread[i];
        }
    }

    std::vector<counters> counters_;

private:
    std::vector<server_worker*> worker;
    zmq::context_t ctx_;
    zmq::socket_t frontend_;
    zmq::socket_t backend_;
};

//  The main thread simply starts several clients and a server, and then
//  waits for the server to finish.

using namespace std;

int main()
{
    cout << "Hello World!" << endl;
    client_task ct1;
    client_task ct2;
    client_task ct3;
    server_task st;

    std::thread t1(std::bind(&client_task::start, &ct1));
    std::thread t2(std::bind(&client_task::start, &ct2));
    std::thread t3(std::bind(&client_task::start, &ct3));
    std::thread t4(std::bind(&server_task::run, &st));

    t1.detach();
    t2.detach();
    t3.detach();
    t4.detach();
    std::this_thread::sleep_for(std::chrono_literals::operator""s(10));
    std::cout << "Client1: " << ct1.counter_rep << ":" << ct1.counter_req << std::endl;
    std::cout << "Client2: " << ct2.counter_rep << ":" << ct2.counter_req << std::endl;
    std::cout << "Client3: " << ct3.counter_rep << ":" << ct3.counter_req << std::endl;
    std::cout << "All clients: " << ct3.counter_rep + ct1.counter_rep + ct2.counter_rep << ":" << ct1.counter_req + ct2.counter_req + ct3.counter_req << std::endl;
    int counter_rep = 0, counter_req = 0;
    for (const auto& counters : st.counters_) {
        counter_rep += *counters.counter_rep;
        counter_req += *counters.counter_req;
        std::cout << "Server thread: " << *counters.counter_rep << ":" << *counters.counter_req << std::endl;
    }
    std::cout << "All server threads: " << counter_rep << ":" << counter_req << std::endl;

    return 0;
}
