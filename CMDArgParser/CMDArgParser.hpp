#pragma once

#include <vector>
#include <unordered_map>
#include <string>
#include <memory>

namespace ZLib {
    namespace CMDArgParser {
        class ParsedValues {
            public:
                std::vector<std::string> flags;
                std::vector<std::string> plaintext;
                std::unordered_map<std::string, std::string> variables;
                
                ParsedValues(
                    std::vector<std::string> flags,
                    std::unordered_map<std::string, std::string> variables,
                    std::vector<std::string> plaintext
                ) {
                    ParsedValues::flags = flags;
                    ParsedValues::variables = variables;
                    ParsedValues::plaintext = plaintext;
                }
        };

        // Parser for parsing the arguments passed to a C++ program
        class Parser {
            private:
                int argc;
                char** argv;

            public:
                Parser(int argc, char** argv) {
                    Parser::argc = argc;
                    Parser::argv = argv;
                }
                
                // Actually parse the values
                std::shared_ptr<ParsedValues> parse() {
                    // To hold our parsed out CLI info
                    std::unordered_map<std::string, std::string> variables;
                    std::vector<std::string> flags;
                    std::vector<std::string> plaintext;
                    
                    //make my life easier       also + 1 is here to remove the filename
                    auto args = std::vector<std::string>{ argv + 1, argv + argc };
                    
                    bool readingVariable = false;
                    std::string varname = "";
                    
                    for (auto item : args) {
                        
                        // If we just read a variable name
                        if (readingVariable) {
                            readingVariable = false;
                            
                            // Add it
                            variables[varname] = item;
                        }
                        
                        // Flag test
                        if (item.rfind("--", 0) == 0) {
                            flags.push_back(item.substr(2));
                        
                        // Variable test
                        } else if (item.rfind("-", 0) == 0) {
                            readingVariable = true;
                            varname = item.substr(1);
                        } else if (!readingVariable) {
                            plaintext.push_back(item);
                        }
                    }
                    
                    // Shared ptrs lmao
                    return std::make_shared<ParsedValues>(flags, variables, plaintext);
                }
        };
    }
}