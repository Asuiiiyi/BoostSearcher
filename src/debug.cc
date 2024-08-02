#include "searcher.hpp"

#include "redis-plus-plus/src/sw/redis++/redis++.h"

#include <iostream>
#include <string>

using namespace sw::redis;

// 测试后端代码，可忽视

const std::string input = "data/raw_html/raw.txt";

int main()
{
    // for test
    // 创建 Redis 客户端对象配置
    ConnectionOptions options;
    options.host = "127.0.0.1";
    options.port = 6379;
    options.password = "123"; 
    Redis redis = Redis(options);

    ns_searcher::Searcher *search = new ns_searcher::Searcher(&redis);
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