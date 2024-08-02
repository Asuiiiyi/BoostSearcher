#include "util.hpp"
#include "log.hpp"

#include <boost/filesystem.hpp>

#include <iostream>
#include <string>
#include <vector>

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