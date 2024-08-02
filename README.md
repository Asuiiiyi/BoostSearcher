# 站内搜索引擎项目

## 1.项目的相关背景

- 网站搜索内容是很有必要的，可以帮你快速定位到你想得到的结果
- 目前有部分网站的站内是没有设计搜索功能，这时候就需要我们动手写一个站内搜索
- 站内搜索：搜索的数据更垂直，数据量更小



**解析网页搜索结果的组成，这里以Microsoft Edge的搜索为例：**

![image-20240611153515981](C:\Users\Asuiiiyi\AppData\Roaming\Typora\typora-user-images\image-20240611153515981.png)

**一个搜索结果主要有三个组成部分：标题、文档摘要、网址URL**



## 2.站内搜索引擎的宏观原理

- 服务器：先将获取到的站内文档保存到Data，调用Parser将文档去标签化，保存到Raw；调用Index构建去标签化文档的索引，至此文档内容完成。
- 客户端：上传关键字，向服务器发送http请求，进行搜索任务。
- 服务器：通用Searcher进行检索索引，得到相关的html，再将得到的多个网页拼接成一个网页，返回给客户端。



## 3.搜索引擎技术和项目环境

- 技术栈：c/c++ ,c++11,STL,准标准库Boost，Jsoncpp，cppjieba，cpp-httplib ，(html5，css，js，jQuery，Ajax 略微涉及)
- 项目环境：Centos 7，vim/gcc(g++)/Makefile，vscode



## 4.正排索引&倒排索引 -搜索引擎Index模块的具体原理

#### 正排索引：通过文档ID找到文档内容

- 文档1[广东财经大学的专业]

- 文档2[中山大学计算机专业]

  

**为了获得文档内容的关键字，我们需要对文档分词：**

- 文档1[广东财经大学的专业]：广东/财经/大学/专业/广东财经大学

- 文档2[中山大学计算机专业]：中山/大学/计算机/专业/中山大学

  

#### 倒排索引：通过文档内容，找到文档内容每个关键词关联到的文档ID

| 关键字（唯一）   | 对应文档ID       |
| ---------------- | ---------------- |
| **广东**         | **文档1**        |
| **财经**         | **文档1**        |
| **大学**         | **文档1，文档2** |
| **专业**         | **文档1，文档2** |
| **广东财经大学** | **文档1**        |
| **中山**         | **文档2**        |
| **计算机**       | **文档2**        |
| **中山大学**     | **文档2**        |

#### 模拟查找过程：

**用户输入：大学 -> 倒排索引中查找 -> 提取出文档ID(1,2) -> 根据正排索引 -> **

**找到文档的内容 -> title+conent+url 文档结果进行摘要->构建响应结果**



## 5. 编写数据去标签与数据清洗的模块 Parser

#### 什么是标签？

```c++
[root@localhost input]# cat yap.html 
<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
<title>Chapter 45. Boost.YAP</title>
<link rel="stylesheet" href="../../doc/src/boostbook.css" type="text/css">
<meta name="generator" content="DocBook XSL Stylesheets V1.79.1">
<link rel="home" href="index.html" title="The Boost C++ Libraries BoostBook Documentation Subset">
<link rel="up" href="libraries.html" title="Part I. The Boost C++ Libraries (BoostBook Subset)">
<link rel="prev" href="xpressive/appendices.html" title="Appendices">
<link rel="next" href="boost_yap/manual.html" title="Manual">
<meta name="viewport" content="width=device-width, initial-scale=1">
</head>
<body bgcolor="white" text="black" link="#0000FF" vlink="#840084" alink="#0000FF">
<table cellpadding="2" width="100%"><tr>
<td valign="top"><img alt="Boost C++ Libraries" width="277" height="86" src="../../boost.png"></td>
<td align="center"><a href="../../index.html">Home</a></td>
<td align="center"><a href="../../libs/libraries.htm">Libraries</a></td>
<td align="center"><a href="http://www.boost.org/users/people.html">People</a></td>
<td align="center"><a href="http://www.boost.org/users/faq.html">FAQ</a></td>
<td align="center"><a href="../../more/index.htm">More</a></td>
.......
    
// <> : html的标签，这个标签对我们进行搜索是没有价值的，需要去掉这些标签，一般标签都是成对出现的

```

#### 在开始编写Parser模块前，先要将待处理的html文档保存在data目录下的input文件中



#### 编写parser.cc

```c++
#include <iostream>
#include <string>
#include <vector>
#include <boost/filesystem.hpp>
#include "util.hpp"

const std::string src_path = "data/input";          // 输入文档数据
const std::string output = "data/raw_html/raw.txt"; // 输出文档数据：去标签、数据清洗

typedef struct DocInfo
{                        // 文档信息
    std::string title;   // 标题
    std::string content; // 内容
    std::string url;     // 文档对应的网页url
} DocInfo_t;

// const &:输入
//*：输出
//&：输入输出

bool EnumFile(const std::string &src_path, std::vector<std::string> *file_list);

bool ParserHtml(const std::vector<std::string> &file_list, std::vector<DocInfo_t> *results);

bool SaveHtml(const std::vector<DocInfo_t> &results, const std::string &output);

int main()
{
    std::vector<std::string> file_list;
    // EnumFile：递归式把所有html文件带路径，保存到file_list里面，方便后期进行文件读取
    if (!EnumFile(src_path, &file_list))
    {
        std::cerr << "enum file error!" << std::endl;
        LOG(FATAL, "遍历文件失败!");
        return 1;
    }
    // ParserHtml：从file_list截取文档信息，并拆分解析，将结果保存到results
    std::vector<DocInfo_t> results; // 存放解析后的结果，vector
    if (!ParserHtml(file_list, &results))
    {
        std::cerr << "parser html error" << std::endl;
        LOG(FATAL, "解析html失败!");
        return 2;
    }
    // SaveHtml：把解析完毕的文件内容，写入output当中，按照\3作为文件之间的分隔符
    if (!SaveHtml(results, output))
    {
        std::cerr << "save html error" << std::endl;
        LOG(FATAL, "保存html失败!");
        return 3;
    }
}

bool EnumFile(const std::string &src_path, std::vector<std::string> *file_list) // EnumFile：递归式把所有html文件带路径，保存到file_list里面，方便后期进行文件读取
{
    namespace fs = boost::filesystem;
    // 创建了一个名为root_path的路径对象，并初始化为src_path的值
    fs::path root_path(src_path);

    // 判断路径是否存在，如果不存在，则没有往下走的必要
    if (!fs::exists(root_path))
    {
        std::cerr << src_path << " not exists" << std::endl;
        return false;
    }

    // 定义一个空的迭代器，用来进行判断递归结束
    fs::recursive_directory_iterator end;
    for (fs::recursive_directory_iterator iter(root_path); iter != end; ++iter)
    {
        // 如果iter指向的文件不是常规文件
        if (!fs::is_regular_file(*iter))
        {
            continue;
        }
        // 如果iter指向的常规文件的后缀名不为“html”
        if (iter->path().extension() != ".html")
        {
            continue;
        }
        // std::cout << "debug:" << iter->path().string() << std::endl;

        // 执行到这里，说明iter指向的文件是以“.html”结尾的常规文件,则将iter指向的path路径转化为string类型，并插入到file_list当中
        file_list->push_back(iter->path().string());
    }
    return true;
}

static bool ParserTitle(const std::string &file, std::string *title) // 解析指定文件。提取title
{
    // 寻找<title>的起点位置
    std::size_t begin = file.find("<title>");
    if (begin == std::string::npos)
    {
        return false;
    }
    // 寻找</title>的起点位置
    std::size_t end = file.find("</title>");
    if (end == std::string::npos)
    {
        return false;
    }
    // begin跳过<title>
    begin += std::string("<title>").size();
    // 这个错误可能性不大
    if (begin > end)
    {
        return false;
    }
    // title替换成file里面的已经去标签之后的内容
    *title = file.substr(begin, end - begin);
    return true;
}
static bool ParserContent(const std::string &file, std::string *content) // 解析指定文件。提取content，本质上是去标签
{
    // 编写状态机：LABLE表示当前字符属于标签，CONTENT表示当前字符属于内容
    enum status
    {
        LABLE,
        CONTENT
    };
    enum status s = LABLE;
    for (char c : file)
    {
        switch (s)
        {
        case LABLE:
            if (c == '>')
                s = CONTENT;
            break;
        case CONTENT:
            if (c == '<')
                s = LABLE;
            else
            {
                // 处理后最好不要保存文件中'\n'
                if (c == '\n')
                    c = ' ';
                // 处理完这些内容后，将字符插入content中
                content->push_back(c);
            }
            break;
        default:
            break;
        }
    }
    return true;
}
static bool ParserUrl(const std::string &file_path, std::string *url) // 解析指定文件的路径。提取url
{
    std::string url_head = "https://www.boost.org/doc/libs/1_85_0/doc/html";
    std::string url_tail = file_path.substr(src_path.size());
    *url = url_head + url_tail;
    return true;
}

bool ParserHtml(const std::vector<std::string> &file_list, std::vector<DocInfo_t> *results) // ParserHtml：从file_list截取文档信息，并拆分解析，将结果保存到results
{
    for (const std::string &file : file_list)
    {
        std::string result;
        // 读取文件，read（）
        if (!ns_util::FileUtil::ReadFile(file, &result))
        {
            continue;
        }
        // std::cout << "debug: " << result << std::endl;
        //  创建文档的信息结构
        DocInfo_t doc;
        // 解析指定文件。提取title
        if (!ParserTitle(result, &doc.title))
        {
            continue;
        }

        // 解析指定文件。提取content，本质上是去标签
        if (!ParserContent(result, &doc.content))
        {
            continue;
        }

        // 解析指定文件的路径。提取url
        if (!ParserUrl(file, &doc.url))
        {
            continue;
        }

        // 文件处理结果保存在doc里
        results->push_back(std::move(doc)); // bug:TODO：5.31:改用move将doc转成右值，减少拷贝开销
        // for debug
        // ShowDoc(doc);
    }
    return true;
}

bool SaveHtml(const std::vector<DocInfo_t> &results, const std::string &output) // SaveHtml：把解析完毕的文件内容，写入output当中，按照\3作为文件之间的分隔符
{
#define SEP '\3'
    // 以二进制方式写入
    std::ofstream out(output, std::ios::out | std::ios::binary);
    if (!out.is_open())
    {
        std::cerr << "open " << output << " failed!" << std::endl;
        LOG(FATAL, "打开" + output + "失败!");
        return false;
    }
    // 成功以后就可以进行文件写入了
    for (auto &item : results)
    {
        // 这里进行字符串拼接，规则：title + \3 + content + \3 + url + \n
        std::string out_string;
        out_string += item.title;
        out_string += SEP;
        out_string += item.content;
        out_string += SEP;
        out_string += item.url;
        out_string += '\n';
        // 写入文件
        out.write(out_string.c_str(), out_string.size());
    }
    // 关闭写入流
    out.close();
    return true;
}
```

#### boost 开发库的安装

```c++
[root@localhost boost_searcher] yum install -y boost-devel   //是boost 开发库
```



## 6. 编写建立索引的模块 Index

#### Index模块需要用到分词工具，这里我们要引入 cppjieba

```c++
[root@localhost boost_searcher] git clone https://gitcode.net/mirrors/yanyiwu/cppjieba.git
```

#### 编写index.hpp

```c++
#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <unordered_map>
#include <mutex>
#include "util.hpp"
#include "log.hpp"
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
```



## 7. 编写搜索引擎模块 Searcher

#### Searcher构建结果需要以json串的形式返还，这里要引入jsoncpp

```c++
[root@localhost boost_searcher] yum install -y jsoncpp-devel
```

#### 编写searcher.hpp

```c++
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

```



## 8. 编写http_server 模块

#### 引入cpp-httplib

```c++
cpp-httplib库：https://gitee.com/zhangkt1995/cpp-httplib?_from=gitee_search

注意：cpp-httplib在使用的时候需要使用较新版本的gcc，centos 7下默认gcc 4.8.5
    
Centos 7更新gcc版本参考：https://github.com/Asuiiiyi/Linux/commit/70e7cdc97a2bda48f83aa58428faf9aa0974b37e

```

#### 编写http_server.cc

```c++
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
    // svr监听8081端口
    svr.listen("0.0.0.0", 8081);
    return 0;
}
```



## 9. 编写前端模块

#### 编写index.html

```html
<!DOCTYPE html>
<html lang="en">

<head>
    <meta charset="UTF-8">
    <meta http-eqiv="X-UA-Compatible" content="IE=edge">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <script src="https://apps.bdimg.com/libs/jquery/2.1.4/jquery.min.js"></script>

    <title>boost 搜索引擎</title>
    <style>
        /*去掉默认内外边距*/
        * {
            margin: 0;
            /*设置外边距*/
            padding: 0;
            /*设置内边距*/
        }

        /* 将我们的body的内容100%吻合 */
        html,
        body {
            height: 100%;
        }

        /* 类选择器 container*/
        .container {
            width: 800px;
            /* 设置div的宽度 */
            margin: 0px auto;
            /* 设置外边距达到居中效果 */
            margin-top: 15px;
            /* 设置外上边距 */
        }

        /* 复合选择器 选中container下的search*/
        .container .search {
            width: 100%;
            /* 宽度与父标签保持一致 */
            height: 52px;
            /* 高度设置52像素点 */
        }

        /* 先选中input标签，直接设置标签的属性，input：标签选择器*/
        .container .search input {
            /* 设置left浮动 */
            float: left;
            width: 600px;
            height: 50px;
            border: 1px solid black;
            /* 设置边框属性：边框宽度、样式、颜色 */
            border-right: none;
            /* 去掉右边框 */
            padding-left: 10px;
            /* 设置左内边距 */
            color: #0000007e;
            /* 设置字体颜色 */
            font-size: 15px;
            /* 设置字体大小 */
            font-family: Georgia, 'Times New Roman', Times, serif;
            /* 设置字体样式 */
        }

        .container .search button {
            float: left;
            /* 设置left浮动 */
            width: 150px;
            height: 52px;
            background-color: #4e6ef2;
            /* 设置button颜色 */
            color: #FFF;
            /* 设置button字体颜色 */
            font-size: 19px;
            /* 设置字体大小 */
            font-family: Georgia, 'Times New Roman', Times, serif;
            /* 设置字体样式 */
        }

        .container .result {
            width: 100%;

        }

        .container .result .item {
            margin-top: 15px;
            /* 设置外上边距 */
        }

        .container .result .item a {
            display: block;
            /*设置a为块级元素，单独一行*/
            text-decoration: none;
            /* 去掉a标签下划线 */
            font-size: 20px;
            /* 设置a标签字体大小 */
            color: #4e6ef2;

        }

        .container .result .item a:hover {
            text-decoration: underline;
            /*设置鼠标放在a上的动态效果*/
        }

        .container .result .item p {
            font-size: 16px;
            /* 设置a标签字体大小 */
            font-family: Georgia, 'Times New Roman', Times, serif;
            /* 设置字体样式 */
            margin-top: 5px;
            /*设置外上边距*/
        }

        .container .result .item i {
            font-style: normal;
            /* 取消斜体风格 */
            color: green;
        }
    </style>
</head>

<body>
    <div class="container">
        <div class="search">
            <input type="text" value="输入搜索关键字..">
            <button onclick="Search()">搜索一下</button>
        </div>
        <div class="result">
            <!-- 动态生成网页 -->
        </div>
    </div>
    <script>
        function Search() {
            //1.提取数据,$理解成JQuery的别称
            let query = $(".container .search input").val();
            console.log("query= " + query);//console是浏览器的对话框，可以用来进行查看

            //2.发起http请求,ajax:前后端数据交互的函数，属于JQuery中的
            $.ajax({
                type: "GET",
                url: "/s?word=" + query,
                success: function (data) {
                    console.log(data);
                    BulidHtml(data);
                }
            })
        }

        function BulidHtml(data) {
            let result_lable = $(".container .result");// 获取html中result标签
            result_lable.empty();//清空历史记录

            for (let elem of data) {
                let a_lable = $("<a>", {
                    text: elem.title,
                    href: elem.url,
                    target: "_blank"//跳转到新的页面
                });

                let p_lable = $("<p>", {
                    text: elem.desc
                });

                let i_lable = $("<i>", {
                    text: elem.url
                });
                let div_lable = $("<div>", {
                    class: "item"
                });
                a_lable.appendTo(div_lable);
                p_lable.appendTo(div_lable);
                i_lable.appendTo(div_lable);
                div_lable.appendTo(result_lable);
            }
        }
    </script>
</body>

</html>
```

#### 实现效果：

![image-20240611165856060](C:\Users\Asuiiiyi\AppData\Roaming\Typora\typora-user-images\image-20240611165856060.png)



## 10.添加日志

```c++
#pragma once
#include <iostream>
#include <string>
#include <ctime>

#define NORMAL 1
#define WARNING 2
#define DEBUG 3
#define FATAL 4

#define LOG(LEVEL, MESSAGE) log(#LEVEL, MESSAGE, __FILE__, __LINE__)

void log(std::string level, std::string message, std::string file, int line) // 编写日志
{
    std::cout << "[" << level << "]" << "[" << time(nullptr) << "]" << "[" << message << "]" << "[" << file << ": " << line << "]" << std::endl;
}
```



## 部署服务到linux上

```c++
[root@localhost boost_searcher] nohup ./http_server > log/log.txt 2>&1 &
[1] 26890

```



## 11.工具类

```c++
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
```



## 说明

- 解决了建立倒排索引时，多个关键词指向同一个文档，进而导致结果出现重复文档的情况

- 分词工具cppjieba还没有深入了解，目前像" ", "_",这类符号在分词后保留，可以会影响搜索结果

- 新增热词统计功能，通过定时线程对redis操作，记录时间内搜索频率较高的词语

  

  
