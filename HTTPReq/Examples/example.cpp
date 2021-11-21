#include "../HTTPReq.hpp"

#include <string>
#include <iostream>

int main(int argc, char const *argv[]) {
    ZLib::Requests::HttpRequest req("http://jsonplaceholder.typicode.com/todos");

    auto res = req.sendRequest("GET");

    std::cout << std::string{ res.body.begin(), res.body.end() } << std::endl;

    return 0;
}
