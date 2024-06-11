#include "searcher.hpp"
#include <iostream>
#include <string>

// 测试后端代码，可忽视

const std::string input = "data/raw_html/raw.txt";

int main()
{
    // for test
    ns_searcher::Searcher *search = new ns_searcher::Searcher();
    search->InitSearcher(input);
    std::string query;
    std::string json_string;
    while (true)
    {
        std::cout << "Please Enter You Search Query#";
        getline(std::cin, query);
        // query.erase(std::remove(query.begin(), query.end(), ' '), query.end());
        search->Search(query, &json_string);
        std::cout << json_string << std::endl;
    }
    return 0;
}