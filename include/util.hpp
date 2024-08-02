#pragma once
#include "log.hpp"

#include <boost/algorithm/string.hpp>
#include "../cppjieba/include/cppjieba/Jieba.hpp"
#include "../redis-plus-plus/src/sw/redis++/redis++.h"

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <chrono>
#include <thread>
#include <set>

using namespace sw::redis;

namespace ns_util
{
    class FileUtil
    {
    public:
        static bool ReadFile(const std::string &file_path, std::string *out) // 读文件
        {
            std::ifstream in(file_path, std::ios::in);
            // 如果打开失败
            if (!in.is_open())
            {
                std::cerr << "open file" << file_path << "error" << std::endl;
                LOG(FATAL, "打开" + file_path + "失败");
                return false;
            }
            // 如果打开成功
            std::string line;
            while (std::getline(in, line)) // 循环，从文件流in里面读取信息，每次读取一行，保存在line里(getline()本身返回类型是引用&，但是内部重载了强制类型转化，所以getline()获取内容为空时，可以返回bool值false)
            {
                // 输出结果返回line的内容
                *out += line;
            }
            // 关闭文件
            in.close();
            return true;
        }
    };
    class StringUtil
    {
    public:
        static void Split(const std::string &target, std::vector<std::string> *out, const std::string sep) // 切分字符串
        {
            // boost split
            boost::split(*out, target, boost::is_any_of(sep), boost::token_compress_on);
        }
    };

    const char *const DICT_PATH = "../dict/jieba.dict.utf8";
    const char *const HMM_PATH = "../dict/hmm_model.utf8";
    const char *const USER_DICT_PATH = "../dict/user.dict.utf8";
    const char *const IDF_PATH = "../dict/idf.utf8";
    const char *const STOP_WORD_PATH = "../dict/stop_words.utf8";

    class JiebaUtil
    {
    private:
        static cppjieba::Jieba jieba; // 分词对象
    public:
        static void CutString(const std::string &src, std::vector<std::string> *out) // 分词
        {
            jieba.CutForSearch(src, *out);
        }
    };
    cppjieba::Jieba JiebaUtil::jieba(DICT_PATH, HMM_PATH, USER_DICT_PATH, IDF_PATH, STOP_WORD_PATH); // 类外初始化分词对象jieba

    class RedisUtil
    {
    public:
        static void AddkeywordAndIncrase(Redis &redis,const std::string &keyword){
            // 将key的时间设为24小时
            //auto one_day_seconds = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::hours(24)).count();
            // 开始事务
            auto pipe = redis.pipeline();
            // 使用管道将命令排队
            pipe.zadd("keyword:famous", keyword, 0, UpdateType::NOT_EXIST);
            // 自增
            pipe.zincrby("keyword:famous", 1, keyword);

            // 执行事务中的所有命令
            pipe.exec();
        }
    };

    class ThreadUtil
    {
    public:
        // 定时器线程函数
        static void timerTask(std::chrono::seconds interval, Redis *redis) {
            while (true) {
                //线程执行sleep，等到唤醒时，就进行对redis的热词数据清洗
                std::this_thread::sleep_for(interval);

                //降序存放统计好的热词
                std::vector<std::pair<std::string, double>>rank;   
                auto iter = std::back_inserter(rank);
                redis->zrevrange("keyword:famous", 0, -1, iter);
                for(auto &e: rank){
                    std::cout << e.first << " "  << e.second << std::endl;
                }

                //清洗存放热词的key
                redis->del("keyword:famous");
                std::cout << "redis热词数据已清洗" << std::endl;
            }
        }
    };
};