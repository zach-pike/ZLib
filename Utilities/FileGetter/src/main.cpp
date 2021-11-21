#include "../../../HTTPReq/HTTPReq.hpp"
#include "../../../CMDArgParser/CMDArgParser.hpp"

#include <fstream>
#include <iostream>
#include <string>
#include <stdlib.h>
#include <time.h>

int main(int argc, char *argv[]) {
    using namespace ZLib;

    // Command line parser
    CMDArgParser::Parser cmdl(argc, argv);
    auto args = cmdl.parse();

    if (args->plaintext.size() < 1) return 0;

    // Make & send req
    Requests::HttpRequest req(args->plaintext[0]);
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
