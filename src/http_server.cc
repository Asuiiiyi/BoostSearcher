#include "searcher.hpp"
#include "log.hpp"

#include "../cpp-httplib/httplib.h"
#include "../redis-plus-plus/src/sw/redis++/redis++.h"

#include <thread> 

using namespace sw::redis;

// http的根目录
const std::string root_path = "../wwwroot";
// 解析html保留的结果
const std::string input = "../data/raw_html/raw.txt";

int main()
{
    // 创建 Redis 客户端对象配置
    ConnectionOptions options;
    options.host = "127.0.0.1";
    options.port = 6379;
    options.password = "123"; 
    Redis redis = Redis(options);

    // 定义定时器间隔，定时器：用于更新redis中存放热词的库，每过interval秒就进行一次清洗
    std::chrono::seconds interval(30); // 30秒
    // 创建并启动定时器线程
    std::thread timerThread(ns_util::ThreadUtil::timerTask, interval, &redis);

    // 初始化Search对象并传入数据
    ns_searcher::Searcher search(&redis);
    search.InitSearcher(input);
    // 创建server对象svr
    httplib::Server svr;
    // 引入http的根目录
    svr.set_base_dir(root_path.c_str());
    // svr获取网页内容
    svr.Get("/s", [&search](const httplib::Request &req, httplib::Response &rsp)
            {
                // 检查 HTTP 请求中是否包含特定查询参数word
                if (!req.has_param("word"))
                {
                    rsp.set_content("必须要有搜索关键字！", "text/plain: chatset=utf-8");
                    return;
                }
                // 如果有就获取
                std::string word = req.get_param_value("word");

                LOG(NORMAL, "用户搜索: " + word);
                std::string json_string;
                //进行搜索
                search.Search(word, &json_string);
                //设定回复内容，MIME类型采用"application/json"
                rsp.set_content(json_string, "application/json");
            });
    LOG(NORMAL, "服务器启动成功...");
    // svr监听
    svr.listen("0.0.0.0", 8081);

    // 等待定时器线程完成（实际中你可能会有其他机制来停止线程）
    timerThread.join();
    
    return 0;
}