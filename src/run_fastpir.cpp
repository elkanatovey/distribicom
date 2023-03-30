
#include <iostream>
#include <unistd.h>
#include <chrono>

#include "fastpir/bfvparams.h"
#include "fastpir/fastpirparams.hpp"
#include "fastpir/client.hpp"
#include "fastpir/server1.hpp"
#include "concurrency/concurrency.h"

void print_usage();
std::vector<std::vector<unsigned char>> populate_db(size_t num_obj, size_t obj_size);

int main(int argc, char *argv[])
{
    size_t obj_size = 0; // Size of each object in bytes
    size_t num_obj = 0;  // Total number of objects in DB

    size_t  num_queries=0;
    size_t num_threads=0;

    int option;
    const char *optstring = "n:s:q:t:";
    while ((option = getopt(argc, argv, optstring)) != -1)
    {
        switch (option)
        {
        case 'n':
            num_obj = std::stoi(optarg);
            break;
        case 's':
            obj_size = std::stoi(optarg);
            break;
        case 'q':
            num_queries = std::stoi(optarg);
            break;
        case 't':
            num_threads = std::stoi(optarg);
            break;
        case '?':
            print_usage();
            return 1;
        }
    }

    if (!num_obj || !obj_size || !num_queries || !num_threads)
    {
        print_usage();
        return 1;
    }
    if(obj_size%2 == 1) {
        obj_size++;
        std::cout<<"FastPIR expects even obj_size; padding obj_size to "<<obj_size<<" bytes"<<std::endl<<std::endl;
    }

    srand(time(NULL));
    std::chrono::high_resolution_clock::time_point time_start, time_end;

    FastPIRParams params(num_obj, obj_size);

    Server server(params);

    //create clients and queries
    std::vector<Client> clients;
    std::vector<PIRQuery> queries;
    std::vector<PIRResponse> answers(num_queries);
    std::vector<uint64_t> indices;
    seal::Blake2xbPRNGFactory factory;
    auto gen =  factory.create();

    for (int i=0; i<num_queries; i++) {
        clients.push_back(Client(params));
        server.set_client_galois_keys(i, clients[i].get_galois_keys());
        uint64_t ele_index = gen->generate()%num_obj; // element in DB at random position
        queries.push_back(clients[i].gen_query(ele_index));   // index of FV plaintext
        indices.push_back(ele_index);
    }


    std::vector<std::vector<unsigned char>> db = populate_db(num_obj, obj_size);
    server.set_db(db);
    ;
    server.preprocess_db();

    std::cout<<"Completed preprocessing db!"<<std::endl<<std::endl;


    concurrency::threadpool* pool = new concurrency::threadpool(num_threads);
    concurrency::safelatch latch(num_queries);
    auto time_pool_s = std::chrono::high_resolution_clock::now();
    for (int j=0; j<num_queries; j++) {

        pool->submit(
                std::move(concurrency::Task{
                        .f = [&, j]() {
                            answers[j] = server.get_response(j, queries[j]);
                            latch.count_down();
                        },
                        .wg = nullptr,
                        .name = "server:generate_reply",
                })
        );

    }
    latch.wait();
    auto time_pool_e = std::chrono::high_resolution_clock::now();
    auto time_pool_us =
            duration_cast<std::chrono::microseconds>(time_pool_e - time_pool_s).count();


    std::cout << "Main: pool query processing time: " << time_pool_us / 1000
         << " ms on "<<num_queries << " queries and "<< num_threads << " threads" << std::endl;

    delete pool;


    for (int j=0; j<num_queries; j++) {
        std::vector<unsigned char> decoded_response = clients[j].decode_response(answers[j], indices[j]);
        bool incorrect_result = false;

        for (int i = 0; i < obj_size ; i++)
        {
            if ((db[indices[j]][i] != decoded_response[i]))
            {
                incorrect_result = true;
                break;
            }
        }

        if (incorrect_result)
        {
            std::cout << "PIR Result is incorrect!" << std::endl<<std::endl;
            return -1;
        }

    }

    std::cout << "PIR results correct!" << std::endl<<std::endl;

}

void print_usage()
{
    std::cout << "usage: main -n <number of objects> -s <object size in bytes> -q <num queries> -t <num threads>" <<
    std::endl;
}


//Populate DB with random elements
//Overload this function to populate DB differently (e.g. from file)
std::vector<std::vector<unsigned char>> populate_db(size_t num_obj, size_t obj_size) {
    std::vector<std::vector<unsigned char>> db(num_obj);
    seal::Blake2xbPRNGFactory factory;
    auto gen =  factory.create();

    for (int i = 0; i < num_obj; i++)
    {
        db[i] = std::vector<unsigned char>(obj_size);
        for (int j = 0; j < obj_size; j++)
        {
            db[i][j] = gen->generate() % 0xFF;
        }
    }
    return db;
}