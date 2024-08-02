#pragma once
#include "util.hpp"
#include "log.hpp"

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <unordered_map>
#include <mutex>

namespace ns_index
{
    struct DocInfo
    {
        std::string title;   // 文档的标签
        std::string content; // 文档的内容
        std::string url;     // 文档对应url
        int doc_id;          // 文档对应的id
    };
    struct InvertedElem
    {
        uint64_t doc_id;  // 文档对应id
        std::string word; // 关键字
        int weight;       // 权重
    };

    // 倒排拉链：出现过关键字的文档id，全部存进来InvertedList
    typedef std::vector<InvertedElem> InvertedList;

    class Index
    {
    private:
        // 正排索引的数据结构用数组，数组的下标对应文档id
        std::vector<DocInfo> forward_index; // 正排索引

        // 倒排索引一定是一个关键字和一组（个）InvertedElem对应
        std::unordered_map<std::string, InvertedList> inverted_index; // 倒排索引
    private:
        Index() {}
        Index(const Index &) = delete;            // 删除拷贝构造
        Index &operator=(const Index &) = delete; // 删除赋值

        static Index *instance; // 单例
        static std::mutex mtx;  // 互斥锁

    public:
        ~Index() {}

    public:
        static Index *GetInstance() // 创建单例模式
        {
            // 如果单例为空,需要创建一个instance
            if (nullptr == instance) // 线程安全，双判定
            {
                mtx.lock(); // 线程安全，加锁操作
                if (nullptr == instance)
                {
                    instance = new Index();
                }
                mtx.unlock(); // 解锁
            }
            return instance;
        }
        DocInfo *GetForwardIndex(uint64_t doc_id) // 根据文档id获得文档内容
        {
            // 文档id不合法
            if (doc_id >= forward_index.size())
            {
                std::cerr << "doc_id out of range,error!" << std::endl;
                LOG(WARNING, "文档id超出范围");
                return nullptr;
            }
            // 返回正排索引中文档id 对应的文档内容
            return &forward_index[doc_id];
        }

        InvertedList *GetInvertedList(const std::string &word) // 根据关键字string，获得倒排拉链
        {
            auto iter = inverted_index.find(word);
            // 没有在 inverted_index 里面找到关于 关键字word 的 倒排拉链InvertedList 的情况
            if (iter == inverted_index.end())
            {
                std::cerr << word << "'s InvertedList not found" << std::endl;
                LOG(WARNING, word + "找不到相关的倒排拉链");
                return nullptr;
            }
            // 返回倒排索引InvertedList_t的地址
            return &(iter->second);
        }

        bool BulidIndex(const std::string &input) // 根据去标签、格式化后的文档，构建正排索引和倒排索引
        // input:parser处理过后的数据源://data/raw_html/raw.txt
        {
            // 打开处理好的文件
            std::ifstream in(input, std::ios::in | std::ios::binary);
            // 打开失败
            if (!in.is_open())
            {
                std::cerr << "sorry, " << input << " open error" << std::endl;
                LOG(WARNING, input + "打开失败!");
                return false;
            }
            // 打开成功
            std::string line;

            // debug
            int count = 0;
            // debug

            // 进行循环读取,每次读取一个文档信息，就为其构建正排索引和倒排索引
            while (std::getline(in, line))
            {
                DocInfo *doc = BulidForwardIndex(line);
                if (doc == nullptr)
                {
                    std::cerr << "bulid " << line << " error!" << std::endl; // for debug
                    continue;
                }
                BulidInvertedIndex(*doc);

                // debug
                count++;
                if (count % 50 == 0)
                {
                    LOG(NORMAL, "当前已经建立的索引文档" + std::to_string(count));
                }
                // debug
            }
            return true;
        }

    private:
        DocInfo *BulidForwardIndex(const std::string &line) // 对单个文档构建正排索引
        {
            // 1.解析line，字符串切分
            const std::string sep = "\3";
            std::vector<std::string> results;
            ns_util::StringUtil::Split(line, &results, sep);
            if (results.size() != 3)
            {

                return nullptr;
            }
            // 2.通过字符串，填充DocInfo：包括title、content、url（此前保存的格式：文件之间\n区分，文件内的数据之间\3区分，所以填充的时候只需要找分隔符就可以快速定位）
            DocInfo doc;
            doc.title = results[0];            // title
            doc.content = results[1];          // content
            doc.url = results[2];              // url
            doc.doc_id = forward_index.size(); // 更新当前文档在正排索引的id
            // 3.插入正排索引的vector：forward_index
            forward_index.push_back(std::move(doc)); //
            // 4.返回当前插入的结构体DocInfo doc，以便接下来构建倒排索引
            return &forward_index.back();
        }
        bool BulidInvertedIndex(const DocInfo &doc) // 对单个文档构建倒排索引
        {
            // 此前通过构建正排索引，获得了文档的拆分信息（包括title、content、url、id）doc
            // DocInfo doc: [title, content, url, id]

            // 词频统计的结构体
            struct word_cnt
            {
                int title_cnt;
                int content_cnt;
                word_cnt() : title_cnt(0), content_cnt(0) {}
            };

            // 用来暂存词频的映射表
            std::unordered_map<std::string, word_cnt> word_map;

            // 对标题进行分词
            std::vector<std::string> title_words;
            ns_util::JiebaUtil::CutString(doc.title, &title_words);

            // 对标题进行词频统计
            for (auto &word : title_words)
            {
                // 将切分好的单词统一成小写
                boost::to_lower(word);
                // 如果不存在就创建，存在就加一
                word_map[word].title_cnt++;
            }

            // 对内容进行分词
            std::vector<std::string> content_words;
            ns_util::JiebaUtil::CutString(doc.content, &content_words);

            // 对内容进行词频统计
            for (auto &word : content_words)
            {
                // 将切分好的单词统一成小写
                boost::to_lower(word);
                // 如果不存在就创建，存在就加一
                word_map[word].content_cnt++;
            }
#define X 10
#define Y 1
            // 遍历映射表，统计单词与文档的相关性
            for (auto &word_pair : word_map)
            {
                InvertedElem item;
                item.doc_id = doc.doc_id;
                item.word = word_pair.first;
                // 相关性
                item.weight = X * word_pair.second.title_cnt + Y * word_pair.second.content_cnt;
                // 插入倒排拉链
                InvertedList &inverted_list = inverted_index[word_pair.first];
                inverted_list.push_back(std::move(item));
            }
            return true;
        }
    };
    Index *Index::instance = nullptr;
    std::mutex Index::mtx;
};