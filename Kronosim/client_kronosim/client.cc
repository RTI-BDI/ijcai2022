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
#include <signal.h> // in order to catch CTRL+C event
#include <vector>
#define PORT 8080

const std::string main_folder_path = "../simulations/francesco/unity_simulations/";
const std::string simulation_folder_name = "__test_client_server_dynamic";
const std::string input_folder = "/inputs/";

std::string read_file_content(std::string file_name);

/*bool send_exit = false;

void signal_handler(int signal)
{
    std::cout << "Caught signal " << signal << std::endl;

    if(signal == 2) { // CTRL + C
        send_exit = true;
    }
}*/

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
    std::string input_command, input_param, command;
    std::string file_name, file_content;

    /* [!] in order to be correctly initialized, the order among files is important!
    * The correct sequence is: beliefset, skillset, desireset, servers, planset, sensors */
    std::vector<std::string> initialization_files = { 
        "beliefset_original.json", "skillset_original.json", "desireset_original.json", "servers_original.json", "planset_original.json", 
        "sensors_original.json", "update-beliefset_original.json" };
    std::vector<std::string> original_names = { "beliefset.json", "skillset.json", "desireset.json", "servers.json", "planset.json",
        "sensors.json", "update-beliefset.json" };

    // signal(SIGINT, signal_handler); // Register signal and signal handler

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

    do
    {
        std::cout << "\nInsert command (help - to print the list of available ones): ";
        std::cin >> input_command;
        input_command = boost::to_upper_copy(input_command);
        memset(&buffer[0], 0, sizeof(buffer)); // clear buffer
        command_already_sent = false;

        if(input_command.compare("HELP") == 0 || input_command.compare("H") == 0) {
            std::cout << "Accepted commands: " << std::endl;
            std::cout << " -> help (or h): list of commands " << std::endl;
            std::cout << " -> invalid (or i): send invalid json without item 'command' (for test) " << std::endl;
            std::cout << " -> exit (or e): exit simulation " << std::endl;
            std::cout << " -> pause (or p): pause simulation " << std::endl;
            std::cout << " -> poke: check if the agent is performing a reasoning cycle " << std::endl;
            // usato solo per test: std::cout << " -> testpoke (or tp): exit the reasoning cycle " << std::endl;
            std::cout << " -> run n (or r n): run until time n " << std::endl;
            std::cout << " -> runall (or ra): run full simulation " << std::endl;
            std::cout << " -> update filename (or u filename): update the knowledge-base of the agent. e.g. update beliefset " << std::endl;
            std::cout << " -> updateall (or ua): update the entire knowledge-base of the agent. " << std::endl;
            std::cout << " -> setup (or s): intial setup of the simulation. Setup the entire knowledge-base of the agent " << std::endl;
        }
        else {
            if(input_command.compare("EXIT") == 0 || input_command.compare("E") == 0) {
                input_command = "EXIT";
                command = "{\"command\": \"UTIL\", \"request\": \""+ input_command +"\"}end_json";
            }
            else if(input_command.compare("INVALID") == 0 || input_command.compare("I") == 0) {
                input_command = "INVALID";
                command = "{\"empty\": \"UTIL\", \"request\": \""+ input_command +"\"}end_json";
            }
            else if(input_command.compare("PAUSE") == 0 || input_command.compare("P") == 0) {
                input_command = "PAUSE";
                command = "{\"command\": \"UTIL\", \"request\": \""+ input_command +"\"}end_json";
            }
            else if(input_command.compare("POKE") == 0) {
                command = "{\"command\": \""+ input_command +"\"}end_json";
            }
            // usato solo per test: 
            //else if(input_command.compare("TESTPOKE") == 0 || input_command.compare("TP") == 0) {
            //    input_command = "TESTPOKE";
            //    command = "{\"command\": \""+ input_command +"\"}end_json";
            //}
            else if(input_command.compare("RUN") == 0 || input_command.compare("R") == 0) 
            {
                std::cout << "Run until time: ";
                std::cin >> input_param;

                input_command = "RUN";
                if(lastTime > stod(input_param)) {
                    std::cout << "Time must be greater than last used: " << lastTime << std::endl;
                }
                else {
                    command = "{\"command\": \""+ input_command +"\", \"time\": "+ input_param +"}end_json";
                }
            }
            else if(input_command.compare("RUNALL") == 0 || input_command.compare("RA") == 0) 
            {
                input_command = "RUNALL";
                command = "{\"command\": \""+ input_command +"\", \"time\": -1 }end_json";
            }
            else if(input_command.compare("UPDATE") == 0 || input_command.compare("U") == 0) // update a single file
            {
                input_command = "UPDATE";
                std::cout << "Name of the file to update: ";
                std::cin >> file_name;

                if(file_name.compare("") != 0 && file_name.length() >= 5) 
                {
                    std::string extension = file_name.substr(file_name.length() - 5, file_name.length());
                    std::cout << extension << std::endl;

                    if(extension.compare(".json") != 0) {
                        file_name = file_name + ".json";
                    }
                    file_content = read_file_content(file_name);
                
                    command = "{\"command\": \""+ input_command 
                    +"\", \"file\": \""+ file_name 
                    +"\", \"content\": "+ file_content 
                    +"}end_json";
                }
            }
            else if(input_command.compare("UPDATEALL") == 0 || input_command.compare("UA") == 0) // update a single file
            {            
                input_command = "UPDATE";
                for(int i = 0; i < (int)original_names.size(); i++)
                {
                    file_name = original_names[i];
                    if(file_name.compare("") != 0 && file_name.length() >= 5) 
                    {
                        std::string extension = file_name.substr(file_name.length() - 5, file_name.length());
                        command_already_sent = true;

                        if(extension.compare(".json") != 0) {
                            file_name = file_name + ".json";
                        }
                        file_content = read_file_content(file_name);
                    
                        command = "{\"command\": \""+ input_command 
                        +"\", \"file\": \""+ file_name 
                        +"\", \"content\": "+ file_content 
                        +"}end_json";

                        send_data = new char[command.length() + 1];
                        strcpy(send_data, command.c_str());

                        send(sock, send_data, strlen(send_data), 0);
                        printf("Message sent...\n");
                        valread = read(sock, buffer, SIZE_MAX_CHAR_ARRAY);
                        printf("> Received: %s\n", buffer);
                        memset(&buffer[0], 0, sizeof(buffer)); // clear buffer
                    }
                }
            }
            else if(input_command.compare("SETUP") == 0 || input_command.compare("S") == 0) // setup the knowledge-base of the agent
            {
                for(int i = 0; i < (int)initialization_files.size(); i++)
                {
                    std::cout << initialization_files[i] << std::endl;
                    if(initialization_files[i].compare("") != 0) 
                    {
                        file_content = read_file_content(initialization_files[i]);
                    
                        command = "{\"command\": \"UTIL\", \"request\" : \"INITIALIZE\", \"file\": \""+ original_names[i] 
                        +"\", \"content\": "+ file_content +"}end_json";

                        send_data = new char[command.length() + 1];
                        strcpy(send_data, command.c_str());

                        send(sock, send_data, strlen(send_data), 0);
                        printf("Message sent...\n");
                        valread = read(sock, buffer, SIZE_MAX_CHAR_ARRAY);
                        printf("> Received: %s\n", buffer);
                        memset(&buffer[0], 0, sizeof(buffer)); // clear buffer
                    }
                }

                input_command = "SETUP_COMPLETED";
                command = "{\"command\": \""+ input_command +"\"}end_json";
            }
            else {
                command = "{\"command\": \""+ input_command +"\"}end_json";
            }

            if(command_already_sent == false) {
                send_data = new char[command.length() + 1];
                strcpy(send_data, command.c_str());

                send(sock, send_data, strlen(send_data), 0);
                printf("Message sent...\n");
                valread = read(sock, buffer, SIZE_MAX_CHAR_ARRAY);
                printf("> Received: %s\n", buffer);
            }
        }
    }
    while(input_command.compare("EXIT") != 0 && strcmp(buffer, "EXIT") != 0); // && send_exit == false);

    return 0;
}

std::string read_file_content(std::string file_name)
{
    std::string file_content = "";
    std::string path = main_folder_path + simulation_folder_name + input_folder + file_name;

    std::ifstream file(path);

    if (!file.fail()) {
        std::ostringstream tmp;
        tmp << file.rdbuf();
        file_content = tmp.str();
        // debug: std::cout << file_content << std::endl;
    }
    else {
        std::cout << "File:["<< file_name <<"] not found!" << std::endl;
    }

    return file_content;
}