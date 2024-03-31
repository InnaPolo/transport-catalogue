#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cassert>
#include <chrono>
#include <string_view>

#include "request_handler.h"

using namespace std;
using namespace std::literals;

void PrintUsage(std::ostream& stream = std::cerr) {
    stream << "Usage: transport_catalogue [make_base|process_requests]\n"sv;
}

int main(int argc, char* argv[]) {

    ostream& stream = cout;
    if (argc != 2) {
        PrintUsage();
        return 1;
    }

    const std::string_view mode(argv[1]);
    bool saving_graph = true;
    if (argc == 3) {
        if (argv[2] == "saving_graph=OFF"sv) {
            saving_graph = false;
        } else {
            PrintUsage(stream);
            return 1;
        }
    }
    if (mode == "make_base"sv)
    {
        transport_catalogue::TransportCatalogue transport_;
        RequestHandler request_handler(transport_,std::cin);
        request_handler.ReadInputDocument();
        request_handler.AddInfo();
        request_handler.CreateGraph();
        request_handler.Serialize(saving_graph);
     
    } else if (mode == "process_requests"sv) {
        //файл вывода
        //std::filebuf file;
        //std::ifstream in("s14_3_opentest_3_process_requests.json");
        //file.open("s14_3_opentest_3_answer.json", std::ios::out);
        //std::ostream out(&file);
        //process requests here
        transport_catalogue::TransportCatalogue transport_;
        RequestHandler request_handler(transport_,std::cin,std::cout);//std::cin,std::cout
        request_handler.ReadInputDocument();
        request_handler.Deserialize(saving_graph);
        request_handler.GetAnswers();
        request_handler.PrintAnswers();
        //file.close();
    } else {
        PrintUsage();
        return 1;
    }
}
