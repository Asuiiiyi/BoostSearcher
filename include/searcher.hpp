#pragma once
#include "index.hpp"
#include "util.hpp"
#include "log.hpp"

#include <jsoncpp/json/json.h>
#include "../redis-plus-plus/src/sw/redis++/redis++.h"

#include <algorithm>
#include <iterator>

using namespace sw::redis;

namespace ns_searcher
{
    // 倒排索引 以文档id 去重后的结果
    struct InvertedElemPrint
    {
        uint64_t doc_id;
        int weight;
        std::vector<std::string> words;
        InvertedElemPrint() : doc_id(0), weight(0) {}
    };

    class Searcher
    {
    private:
        ns_index::Index *index; // 供系统进行查找的索引
        Redis *redis;
    public:
        Searcher(Redis* Server) 
        :redis(Server)
        {
            std::cout << redis->ping() << std::endl;//DEBUG
        }
        ~Searcher() {}

    public:
        void InitSearcher(const std::string &input) // 初始化Searcher
        {
            // 1.获取或创建Index单例对象
            index = ns_index::Index::GetInstance();
            LOG(NORMAL, "获取index单例成功...");

            // 2.根据Index对象建立索引
            if (index->BulidIndex(input))
            {
                LOG(NORMAL, "根据Index对象建立索引成功...");
            }
        }
        // query：需要搜索的关键字      json_string:返回给用户浏览器的搜索结果
        void Search(const std::string &query, std::string *json_string) // 搜索功能
        {
            // 1.[分词]:对我们的query进行按照searcher的要求进行分词
            // words：存放分词结果
            std::vector<std::string> words;
            ns_util::JiebaUtil::CutString(query, &words);

            // 2.[触发]:根据分词的结果（切分出来的词），进行index查找
            // inverted_list_all：用来最后整合 切分词对应倒排拉链 的每个InvertedElemPrint
            std::vector<InvertedElemPrint> inverted_list_all;
            std::unordered_map<uint64_t, InvertedElemPrint> tokens_map;

            // 遍历分好的词
            for (std::string word : words)
            {
                // 统一小写
                boost::to_lower(word);
                // 调用index获取关于word的倒排拉链
                ns_index::InvertedList *inverted_list = index->GetInvertedList(word);
                // 如果这个词找不到相关的倒排拉链，直接略过
                if (nullptr == inverted_list)
                {
                    continue;
                }
                // 如果找到了,直接从inverted_list_all的尾部插入关键字word的倒排拉链的内容（InvertedElemPrint）
                // 整个过程相当于把你要搜索的关键字拆分，然后逐个搜索相关的倒排拉链，然后把每一个倒排拉链里面的元素重新整合到一起（整合到inverted_list_all）

                //新增热词统计：
                ns_util::RedisUtil::AddkeywordAndIncrase(*redis, word);

                // 去重过程：id重复的就进行权重(weight)累加，关键字添加到vector
                for (const auto &elem : *inverted_list)
                {
                    auto &item = tokens_map[elem.doc_id];
                    // item一定是doc_id相同的print节点
                    item.doc_id = elem.doc_id;
                    item.weight += elem.weight;
                    item.words.push_back(elem.word);
                }
            }
            for (const auto &item : tokens_map)
            {
                inverted_list_all.push_back(std::move(item.second));
            }

            // 3.[合并排序]:汇总查找的结果，并进行相关性（weight）的降序排序
            std::sort(inverted_list_all.begin(), inverted_list_all.end(), [](const InvertedElemPrint &e1, const InvertedElemPrint &e2)
                      { return e1.weight > e2.weight; });

            // 4.[构建]:根据查找出来的每个结果，构建json串（需要引入jsoncpp）
            Json::Value root;

            for (auto &item : inverted_list_all)
            {
                ns_index::DocInfo *doc = index->GetForwardIndex(item.doc_id);
                if (nullptr == doc)
                {
                    continue;
                }

                Json::Value elem;
                elem["title"] = doc->title;
                elem["desc"] = GetDesc(doc->content, item.words[0]);
                elem["url"] = doc->url;

                // 追加到root
                root.append(elem);
            }
            //Json::FastWriter writer;
            Json::StyledWriter writer;
            // 生成序列化并返回到json_string
            *json_string = writer.write(root);
        }

        std::string GetDesc(const std::string &html_content, const std::string &word) // 获取摘要
        {
            // 找到hetm_content中word首次出现的位置，前后分别截取部分内容作为desc
            const int prev_stop = 50;
            const int next_stop = 100;
            // 1.找到首次出现的位置
            auto iter = std::search(html_content.begin(), html_content.end(), word.begin(), word.end(), [](int x, int y)
                                    { return (std::tolower(x) == std::tolower(y)); });
            if (iter == html_content.end())
            {
                LOG(WARNING, "出现与关键字不符的文档");
                return "None";
            }
            int pos = std::distance(html_content.begin(), iter);
            // 2.获取begin和end
            int start = 0;
            int end = html_content.size() - 1;
            // 如果前面不止50个字符，更新start
            if (pos > prev_stop)
            {
                start = pos - prev_stop;
            }
            // 如果后面不止100个字符，更新end
            if (pos + next_stop < end)
            {
                end = pos + next_stop;
            }
            // 3.在范围内截取子字符串
            if (start >= end)
            {
                return nullptr;
            }
            return html_content.substr(start, end - start);
        }
    };
}
