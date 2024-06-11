#include "searcher.hpp"
#include "log.hpp"
#include "cpphttplib/httplib.h"
// http的根目录
const std::string root_path = "./wwwroot";
// 解析html保留的结果
const std::string input = "data/raw_html/raw.txt";

int main()
{
    ns_searcher::Searcher search;
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
    return 0;
}