#include "../../../HTTPReq/HTTPReq.hpp"
#include "../../../CMDArgParser/CMDArgParser.hpp"

#include <fstream>
#include <iostream>
#include <string>
#include <stdlib.h>
#include <time.h>

#include "CxxUrl/url.hpp"

ZLib::Requests::ConnectionInfo URLToConnectionInfo(const Url& url) {
    return ZLib::Requests::ConnectionInfo{
        .port = 80,
        .path = url.path(),
        .hostname = url.host()
    };
}

int main(int argc, char *argv[]) {
    using namespace ZLib;

    // Command line parser
    CMDArgParser::Parser cmdl(argc, argv);
    auto args = cmdl.parse();

    if (args->plaintext.size() < 1) return 0;

    // load url
    Url url(args->plaintext[0]);

    // Make & send req
    Requests::HttpRequest req(URLToConnectionInfo(url));
    auto res = req.sendRequest("GET");

    // Dest file location
    std::string dest;

    if (args->variables.count("out") > 0) 
        dest = args->variables.at("out");
    else {
        srand(time(NULL));

        dest = std::to_string(rand());
    }


    // Write to the dest
    std::ofstream outfile(dest);
    outfile << std::string{ res.body.begin(), res.body.end() };
    outfile.close();

    return 0;
}
