#pragma once
#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <boost/algorithm/string.hpp>
#include "cppjieba/Jieba.hpp"
#include "log.hpp"
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

    const char *const DICT_PATH = "./dict/jieba.dict.utf8";
    const char *const HMM_PATH = "./dict/hmm_model.utf8";
    const char *const USER_DICT_PATH = "./dict/user.dict.utf8";
    const char *const IDF_PATH = "./dict/idf.utf8";
    const char *const STOP_WORD_PATH = "./dict/stop_words.utf8";

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
};