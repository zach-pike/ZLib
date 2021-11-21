#include "../HTTPReq.hpp"
#include <fstream>
#include <iostream>

int main(int argc, char const *argv[]) {
    using namespace ZLib::Requests;

    ConnectionInfo info = {
        .port = 80,
        .path = "/todos",
        .hostname = "jsonplaceholder.typicode.com"
    };
    
    HttpRequest req(info);

    auto resp = req.sendRequest("GET");

    printf("%s\n\n", resp.header.c_str());

    for (char let : resp.body) putchar(let);

    std::ofstream file("todos.json");
    file << std::string{ resp.body.begin(), resp.body.end() };
    file.close();

    return 0;
}
