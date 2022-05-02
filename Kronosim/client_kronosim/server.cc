// ***********************************************************************
// TUTORIAL: https://www.geeksforgeeks.org/socket-programming-cc/
// ***********************************************************************

//////////////////////////////////////////////////////////////////////////
// [!] This server is just for testing, it's different from the one using in Kronosim
//////////////////////////////////////////////////////////////////////////

// Server side C/C++ program to demonstrate Socket programming
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <string>
#include <iostream> // std::cout, cin -- use g++ instead of gcc in order to use it

#define PORT 8080

int main(int argc, char const *argv[])
{
    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024 * 100] = {0};
    // char *hello = "Hello from server";
       
    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
       
    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( PORT );
       
    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, 
                       (socklen_t*)&addrlen))<0)
    {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    int tot_files = 0, tot_split = 0;
    int received_chars = 0;
    std::string resp = "", rec_msg = "", end_msg = "";
    while(strcmp(buffer, "EXIT") != 0)
    {
        memset(buffer, 0, strlen(buffer));
        valread = read(new_socket , buffer, 1024 * 100);
        printf("Received: [%i]\n", valread);
        
        received_chars += strlen(buffer);
        rec_msg += buffer;

        std::cout << "received_chars: "<< received_chars << std::endl;
        std::cout << "rec_msg.size() >= 8: "<< rec_msg.size() << std::endl;
        if(rec_msg.size() >= 8) // 8 is the length of keyword "end_json"
        {
            end_msg = rec_msg.substr(rec_msg.size() - 8, rec_msg.size());
            std::cout << " > end_msg:["<< end_msg << "]" << std::endl;
            if(end_msg.compare("end_json") != 0) {
                std::cout << " [ERROR] Received["<< valread <<"] message does not contain end of file keyword ('end_json')! --> waiting..." << std::endl;
                tot_split += 1;
            }
            else { // the end of the message contains the keyword "end_json"str
                tot_files += 1;
                const char * c_resp = (std::to_string(received_chars)).c_str();
                send(new_socket, c_resp, strlen(c_resp), 0 );
                printf("Responded to client...\n");

                resp = ""; rec_msg = ""; end_msg = "";
                received_chars = 0;
            }
        }
        else if(rec_msg.size() == 0) {
            std::cerr << "Client NOT connected anymore...." << std::endl;
            break;
        }
    }

    std::cout << "tot_files: "<< tot_files << ", tot_split: "<< tot_split << std::endl;
    return 0;
}

