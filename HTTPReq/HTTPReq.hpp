#pragma once

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <vector>
#include <string>
#include <unordered_map>
#include <algorithm>

namespace ZLib {
    namespace Requests {
        struct ConnectionInfo {
            uint16_t port;
            std::string path;
            std::string hostname;
        };

        struct Response {
            std::string header;
            std::vector<char> body;
        };

        // A class for managing HTTP request headers
        class Headers {
            private:
                std::unordered_map<std::string, std::string> headers;
                
            public:

                // Add a header to the header group
                void addHeader(std::string name, std::string value) {
                    headers[name] = value;
                }
                
                // Retrive a header
                std::string getHeader(std::string name) const {
                    return headers.at(name);
                }
                
                // Construct all the headers into a string
                std::string construct() const {
                    std::string cHeaders = "";
                    
                    for (auto header : headers) {
                        cHeaders += header.first;
                        cHeaders += ": ";
                        cHeaders += header.second;
                        cHeaders += "\r\n";
                    }
                    
                    return cHeaders;
                }
        };

        // A class for managing the Request line of the HTTP request
        class RequestLine {
            private:
                std::string method;
                std::string path;
                std::string httpVersion;
                
            public:
                RequestLine(std::string method, std::string path, std::string version) {
                    RequestLine::method = method;
                    RequestLine::path = path;
                    RequestLine::httpVersion = version;
                }
                
                std::string construct() const {
                    // Construct request line
                    std::string requestLine = method;
                    requestLine += " ";
                    requestLine += path;
                    requestLine += " HTTP/";
                    requestLine += httpVersion;
                    
                    return requestLine;
                }
        };

        // For Core library purposes
        namespace Core {
            // Construct a HTTP request
            std::string makeRequest(const RequestLine& reqLine, const Headers& headers, const std::vector<char>& body) {
                std::string request = reqLine.construct();
                request += "\r\n";
                request += headers.construct();
                request += "\r\n";
                request += std::string{ body.begin(), body.end() };
                
                return request;
            }

            size_t unsafelyParseHeaderForLength(std::string header) {
                size_t len = 0;
                
                // Convert header string to lowercase
                transform(header.begin(), header.end(), header.begin(), ::tolower);
                
                // To hold our split string
                std::vector<std::string> split;
                
                // Split code stolen from SOF
                size_t sPos = 0;
                std::string token;
                while((sPos = header.find("\n")) != std::string::npos) {
                    token = header.substr(0, sPos);
                    split.push_back(token);
                    header.erase(0, sPos + 1);
                }
                
                // iterate over thoes chunks
                for (auto header : split) {
                    // Check if the item starts with content-length
                    if (header.rfind("content-length:", 0) == 0) {
                        // To store index of number start/end
                        size_t numStart = 0;
                        size_t numEnd = 0;
                        
                        for (unsigned i=0; i < header.size(); i++) {
                            auto let = header[i];
                            
                            // If letter is a number than if we have not already, mark the beginning of the size
                            if (let >= '0' && let <= '9') {
                                if (numStart == 0) {
                                    numStart = i;
                                }
                            // If we read something thats not a number and the start is not zero, record the end pos
                            } else if (numStart != 0) {
                                // We have reached the end of the string
                                numEnd = i;
                            }
                        }
                        
                        // Construct the number
                        auto numStr = header.substr(numStart, numEnd - numStart);
                        
                        // Set length to size string
                        len = stoul(numStr);
                        
                        // exit loop
                        break;
                    }
                }
                
                return len;
                
            }

            // For next function
            enum class BodyEncodingType {
                NORMAL,
                CHUNKED
            };

            // Parses a broken body and gets the full response
            std::vector<char> bodyParser(int sockfd, std::string header, std::vector<char> prepend ) {
                // First we need to decide what kind of encoding this is
                
                // check if headers say that Transfer-Encoding: chunked and if it is we are dealing with chunked data
                BodyEncodingType type;
                
                // Determine the transfer encoding
                if (header.find("Transfer-Encoding: chunked") != std::string::npos) {
                    type = BodyEncodingType::CHUNKED;

                } else if (header.find("Content-Length: ") != std::string::npos) {
                    type = BodyEncodingType::NORMAL;
                } else {
                    throw "Unknow body encoding type";
                }
                
                
                // Normal encoding, Aka Content-Length
                if (type == BodyEncodingType::NORMAL) {
                    // Parse thru headers and try and find the length
                    size_t content_length = unsafelyParseHeaderForLength(header);
                    
                    // Determine how much of the body we have alread read
                    size_t bytesOfBodyRead = prepend.size();
                
                    // If we have not read the entire body read the rest of it
                    // Implement a function to read the rest of the body if we have not
                    // read up to the content length
                    if (bytesOfBodyRead < content_length) {
                        // We have more data to read
                        // 1 MB buffer
                        char bodybuf[1000000];
                        
                        for (unsigned i=0; i < sizeof(bodybuf); i++) bodybuf[i] = 0;
                        
                        // Copy in the overhang
                        memcpy(bodybuf, prepend.data(), prepend.size());
                        
                        // Read untill we have read all of the body
                        for(;;) {
                            // Read some more data
                            bytesOfBodyRead += recv(sockfd, bodybuf + bytesOfBodyRead, sizeof(bodybuf), 0);
                            
                            // If we have read all the data, breaak out of the loop
                            if (bytesOfBodyRead == content_length) break;
                        }
                        
                        // Set body to the new ntire body
                        prepend = std::vector<char>{ bodybuf, bodybuf + bytesOfBodyRead };
                    }
                    
                    // Return the body
                    return prepend;

                    // Chunked encoding
                } else if (type == BodyEncodingType::CHUNKED) {
                    // 1 MB response buffer
                    char bodybuf[1000000];
                    // Initialize the buffer to zero
                    for (unsigned i=0; i < sizeof(bodybuf); i++) bodybuf[i] = 0;
                    
                    // Now we move any already read body into the buffer
                    memcpy(bodybuf, prepend.data(), prepend.size());
                    
                    // initialize the bytes read value to the size of the overhang
                    size_t bytesRead = prepend.size();
                    
                    for (;;) {
                        // Read as many bytes as possible
                        bytesRead += recv(sockfd, bodybuf + bytesRead, sizeof(bodybuf), 0);
                        
                        // Check if we reach the end block
                        if (strstr(bodybuf, "0\r\n\r\n")) break;
                    }
                    
                    // Make a string out of the read body
                    // Also this is only used to use std::string::find lmao
                    std::string bodyString{ bodybuf, bodybuf + bytesRead};
                    
                    // Final body
                    std::vector<char> body;

                    // Not used but contains the total size of the transferred data in bytes once done
                    size_t size = 0;
                    
                    for (;;) {
                        // Find where the size value ends
                        size_t sizeEnd = bodyString.find("\r\n");

                        // Make a substring of the size
                        auto sizeStr = bodyString.substr(0, sizeEnd);
                        
                        // Read the value into a size
                        size_t blockSize = stoi(sizeStr, nullptr, 16);

                        // Add bytes to size
                        size += blockSize;
                        
                        // if we read the 0 we know we have finished reading the body 
                        if (blockSize == 0) break;
                        
                        // now read blocksize ammount of data after the \r\n
                        auto data = bodyString.substr(sizeEnd + 2, blockSize);
                        
                        // printf("Data: %s\n", data.c_str());
                        
                        body.insert(body.end(), data.begin(), data.end());
                        
                        // Remove all info about the block, the size,
                        // the first and the last CR+LF and the entire 
                        // block
                        bodyString.erase(0, sizeStr.size() + 4 + blockSize);
                    }
                    
                    // Return our body vector
                    return body;
                }

                return std::vector<char>{};
            }

            // Takes a given hostname and converts it to a ip address to be used by inet_addr()
            const char* hostToIP(const char* host) {
                auto he = gethostbyname(host);
        
                const char* ip = inet_ntoa(*(struct in_addr*)he->h_addr);

                return ip;
            }
        }

        class HttpRequest {
            private:
                ConnectionInfo conninfo;

            public:
                HttpRequest(ConnectionInfo info) {
                    conninfo = info;
                }

                // Btw, Host header gets set by send() function, no worries
                Headers headers;

                // Send a HTTP request with a optional body
                Response sendRequest(const char* method, std::vector<char> body = {}) {
                    const char* ip = Core::hostToIP(conninfo.hostname.c_str());

                    // Asseble the request line
                    RequestLine reqline(method, conninfo.path, "1.1");

                    headers.addHeader("Host", conninfo.hostname);
                    headers.addHeader("Accept", "*/*");
                    headers.addHeader("Connection", "close");

                    // If we have a header
                    if (body.size() > 0) headers.addHeader("Content-Length", std::to_string(body.size()));

                    // Construct the full request
                    auto request = Core::makeRequest(reqline, headers, body);

                    struct sockaddr_in server_addr;
        
                    // Construct the server address
                    server_addr.sin_family = AF_INET;
                    server_addr.sin_port = htons(conninfo.port);
                    server_addr.sin_addr.s_addr = inet_addr(ip);
                    
                    // Make a TCP socket
                    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
                    
                    // Connect to the server
                    connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
                    
                    send(sockfd, request.c_str(), request.size(), 0);
                    
                    // Recive till double \r\n indicating end of headers
                    size_t readBytes = 0;
                    
                    // Our response buffer
                    char buf[250000];
                    for (unsigned i=0; i < 250000; i ++) buf[i] = 0;
                    
                    for (;;) {
                        readBytes += recv(sockfd, buf + readBytes, sizeof(buf), 0);
                        
                        if (strstr(buf, "\r\n\r\n")) break;
                    }
                    
                    // End of headers index + the double CRLF
                    size_t headEnd = (strstr(buf, "\r\n\r\n") - buf) + 4;
                    
                    // Now we have found the end of the header, how much of the body
                    // did we accedentally read
                    size_t overhang = readBytes - headEnd;
                    
                    // Construct a string with the header pos
                    std::string respHeaders{ buf, (buf + headEnd) - 4 };
                    
                    // read the initial body overhang
                    std::vector<char> bodyOverhang{ buf + headEnd, buf + readBytes };
                    
                    // Parse the body
                    auto respBody = Core::bodyParser(sockfd, respHeaders, bodyOverhang);
                    
                    // Return the data
                    return Response{ respHeaders, respBody };
                }
        };

    }
}