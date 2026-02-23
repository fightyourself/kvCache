#include <vector>
#include <string>
#include <iostream>
#include "raft.h"
int main(int argc, char *argv[]){
    if(argc!=2){
        std::cout<< "usage: ./main [id]" <<std::endl;
        return 0;
    }
    std::vector<std::string> addrs{"127.0.0.1:5000","127.0.0.1:5001","127.0.0.1:5002"};
    int me = atoi(argv[1]);
    Raft raft(addrs,me);
    raft.run();
    return 0;
}