// ***********************************************************************
// TUTORIAL: https://www.geeksforgeeks.org/socket-programming-cc/
// ***********************************************************************
// Client side C/C++ program to demonstrate Socket programming

// HOW TO RUN: 
// ..$ g++ client.cc -o client
// ..$ ./client

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <exception> // in order to do try {} catch (exception& e) {}
#include <fstream> // to read, write files
#include <iostream> // std::cout, cin -- use g++ instead of gcc in order to use it
#include <sstream>
#include <chrono> // timespan(...)
#include <thread> // sleep_for(...)
#include <boost/algorithm/string.hpp> // to_upper_copy(...)
#include <vector>

#include <glob.h> // get the list of folders in path

#include "json.hpp" // to read from json file
using json = nlohmann::json;
#define PORT 8080


const char* main_folder_path = "../simulations/francesco/24-08-21-generated_simulations/*";
const std::string simulation_folder_name = "";
const std::string input_folder = "/inputs/";

std::string read_file_content(std::string main_path, std::string file_name);

int main(int argc, char const *argv[])
{
    bool connection_established = false;
    bool command_already_sent = false;

    std::chrono::milliseconds sleep_interval(1000); // 1 second sleep
    int sock = 0, valread;
    const int SIZE_MAX_CHAR_ARRAY = 1024 * 100;
    double lastTime = 0;
    struct sockaddr_in serv_addr;
    char *send_data;
    char buffer[SIZE_MAX_CHAR_ARRAY] = {0};
    std::string input_command = "", input_param = "", command = "";
    std::string file_name = "", file_content = "";
    std::string buffer_str = "";
    json server_resp;

    std::vector<std::string> json_names = { "beliefset.json", "skillset.json", "desireset.json", "servers.json", "planset.json",
        "sensors.json", "update-beliefset.json" };

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("Socket creation error! \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0)
    {
        printf("Invalid address/ Address not supported! \n");
        return -1;
    }

    while(connection_established == false)
    {
        if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        {
            printf("Connection Failed! \n");
            std::this_thread::sleep_for(sleep_interval);
        }
        else {
            printf("Connection Established! \n");
            connection_established = true;
        }
    }

    glob_t glob_result, inner_glob_result;
    // compute the list of folders in "main_folder_path"
    glob(main_folder_path, GLOB_TILDE, NULL, &glob_result);

    int tot_files = 0; 
    int tot_true = 0;
    int tot_false = 0;

    // loop over all folders
    for(int i = 0; i < glob_result.gl_pathc; i++)
    {
        char* in_path = glob_result.gl_pathv[i];
        strcat(in_path, "/*");

        glob(in_path, GLOB_TILDE, NULL, &inner_glob_result);

        for(int j = 0; j < inner_glob_result.gl_pathc; j++)
        {
            for(int y = 0; y < json_names.size(); y++) 
            {
                tot_files += 1;
                file_content = read_file_content(inner_glob_result.gl_pathv[j], json_names[y]);

                command = "{\"command\": \"UTIL\", \"request\" : \"INITIALIZE\", \"file\": \""+ json_names[y] +"\", \"content\": "+ file_content +"}end_json";

                send_data = new char[command.length() + 1];
                strcpy(send_data, command.c_str());

                // std::cout << command.length() << ", " << ", " << command.size() << std::endl;

                send(sock, send_data, strlen(send_data), 0);
                // printf("Message sent...[%li]\n", strlen(send_data));
                valread = read(sock, buffer, SIZE_MAX_CHAR_ARRAY);

                buffer_str = buffer;
                if(buffer_str.compare(std::to_string(command.length())) == 0) {
                    printf("> Received: %s, %li, TRUE\n", buffer, command.length());
                    tot_true += 1;
                }
                else {
                    printf("> Received: %s, %li, FALSE\n", buffer, command.length());
                    tot_false += 1;
                }

                memset(&buffer[0], 0, sizeof(buffer)); // clear buffer
            }
        }
    }

    std::cout << "tot: "<< tot_files << ", true: "<< tot_true << ", false: "<< tot_false << std::endl;
    return 0;
}

std::string read_file_content(std::string main_path, std::string file_name)
{
    std::string file_content = "";
    std::string path = main_path + input_folder + file_name;

    std::ifstream file(path);

    if (!file.fail()) {
        std::ostringstream tmp;
        tmp << file.rdbuf();
        file_content = tmp.str();
    }
    //else {
    //    std::cout << "File:["<< file_name <<"] not found!" << std::endl;
    //}

    return file_content;
}
