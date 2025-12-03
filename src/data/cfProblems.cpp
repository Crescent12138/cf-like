#include "cfProblems.h"
#include <algorithm>
#include <ctime>
#include <fstream>
#include <cstdlib>
#include <butil/logging.h>
#include <json2pb/rapidjson.h>

namespace suggest{
    
std::shared_ptr<CfProblems> CfProblemHandler::list_ptr = nullptr;

const std::string CfProblems::PROBLEMS_FILE_PATH = std::string(getenv("HOME")) + "/cf-like/cf_problems_cache.json";

bool CfProblems::LoadFromLocalFile() {
    if (!utils::FileUtils::FileExists(PROBLEMS_FILE_PATH)) {
        LOG(INFO) << "Local problems file not found: " << PROBLEMS_FILE_PATH;
        return false;
    }
    
    std::string content;
    if (!utils::FileUtils::ReadFileContent(PROBLEMS_FILE_PATH, content)) {
        LOG(WARNING) << "Failed to read local problems file";
        return false;
    }
    
    if (content.empty()) {
        LOG(WARNING) << "Local problems file is empty";
        return false;
    }
    
    // 解析JSON
    butil::rapidjson::Document doc;
    if (doc.Parse(content.c_str()).HasParseError()) {
        LOG(ERROR) << "Failed to parse local problems file";
        return false;
    }
    
    if (!doc.HasMember("problems") || !doc["problems"].IsArray()) {
        LOG(ERROR) << "Invalid format in local problems file";
        return false;
    }
    
    if (!doc.HasMember("problemStatistics") || !doc["problemStatistics"].IsArray()) {
        LOG(ERROR) << "Invalid format in local problems file - missing problemStatistics";
        return false;
    }
    
    // 构建solved count映射
    std::map<std::string, int> problem_solved;
    auto& statistics = doc["problemStatistics"];
    for (auto i = 0; i < statistics.Size(); i++) {
        int contestId;
        std::string index;
        int solved;
        if (utils::get_rapidjon_int(statistics[i], "contestId", contestId) &&
            utils::get_rapidjon_string(statistics[i], "index", index)) {
            utils::get_rapidjon_int(statistics[i], "solvedCount", solved, false);
            std::string problem_ID = std::to_string(contestId) + index;
            problem_solved[problem_ID] = solved;
        }
    }
    
    auto& problems = doc["problems"];
    ParseProblemsFromJson(problems, problem_solved);
    
    LOG(INFO) << "Loaded " << problemList.size() << " problems from local file";
    return problemList.size() > 0;
}

void CfProblems::LoadFromCodeforces() {
    std::map<std::string, int> problem_solved;
    CfClient client{};
    auto& doc = client.doc;
    
    if (!doc.HasMember("status") || !doc["status"].IsString()) {
        LOG(WARNING) << "Invalid CF response format";
        return;
    }
    
    std::string status = doc["status"].GetString();
    if (status != "OK") {
        LOG(ERROR) << "CF API status: " << status;
        return;
    }
    
    // 获取解决统计信息
    if (doc.HasMember("result") && doc["result"].HasMember("problemStatistics")) {
        auto& problems = doc["result"]["problemStatistics"];
        if (problems.Size() > 0) {
            LOG(INFO) << "Problem statistics size: " << problems.Size();
        }
        for (auto i = 0; i < problems.Size(); i++) {
            int contestId;
            std::string index;
            int solved;
            if (utils::get_rapidjon_int(problems[i], "contestId", contestId) &&
                utils::get_rapidjon_string(problems[i], "index", index)) {
                utils::get_rapidjon_int(problems[i], "solvedCount", solved, false);
                std::string problem_ID = std::to_string(contestId) + index;
                problem_solved[problem_ID] = solved;
            }
        }
    }
    
    // 获取题目信息
    if (doc.HasMember("result") && doc["result"].HasMember("problems")) {
        auto& problems = doc["result"]["problems"];
        if (problems.Size() > 0) {
            LOG(INFO) << "Problems size: " << problems.Size();
            ParseProblemsFromJson(problems, problem_solved);
            
            // 保存到本地文件
            if (SaveToLocalFile()) {
                LOG(INFO) << "Successfully saved problems to local file";
            } else {
                LOG(WARNING) << "Failed to save problems to local file";
            }
        }
    } else {
        LOG(WARNING) << "No problems in CF response";
    }
    
    if (map_.size() > 0) {
        LOG(INFO) << "problems_map_size: " << map_.size();
    } else {
        LOG(WARNING) << "problems_Map is empty!!!";
    }
}

void CfProblems::ParseProblemsFromJson(const butil::rapidjson::Value& problems, 
                                      const std::map<std::string, int>& problem_solved) {
    for (auto i = 0; i < problems.Size(); i++) {
        Feed feed;
        std::string name;
        int contestId;
        std::string index;
        int rating;
        
        if (!utils::get_rapidjon_string(problems[i], "name", name) ||
            !utils::get_rapidjon_int(problems[i], "contestId", contestId) ||
            !utils::get_rapidjon_string(problems[i], "index", index)) {
            continue;
        }
        
        utils::get_rapidjon_int(problems[i], "rating", rating, false);
        
        std::string problem_ID = std::to_string(contestId) + index;
        std::vector<std::string> tags;
        utils::get_rapidjon_vector_string(problems[i], "tags", tags);
        
        std::vector<std::string> analyzed_tags;
        utils::get_rapidjon_vector_string(problems[i], "analyzed_tags", analyzed_tags);
        
        feed.set_id(problem_ID);
        feed.set_title(name);
        feed.set_rating(rating);
        
        auto it = problem_solved.find(problem_ID);
        if (it != problem_solved.end()) {
            feed.set_solved(it->second);
        } else {
            feed.set_solved(0);
        }
        
        for (const auto& tag : tags) {
            feed.add_tag(tag);
        }
        
        for (const auto& analyzed_tag : analyzed_tags) {
            feed.add_analyzed_tag(analyzed_tag);
        }
        
        const std::string problem_url =
            "https://codeforces.com/problemset/problem/" + std::to_string(contestId) + "/" + index;
        feed.set_url(problem_url);
        
        map_[feed.id()] = feed;
        problemList.emplace_back(std::make_shared<Feed>(feed));
    }
    
    LOG(INFO) << "Parsed " << problemList.size() << " problems";
}

bool CfProblems::SaveToLocalFile() {
    butil::rapidjson::Document doc;
    doc.SetObject();
    auto& allocator = doc.GetAllocator();
    
    // 创建problems数组
    butil::rapidjson::Value problems(butil::rapidjson::kArrayType);
    butil::rapidjson::Value problemStatistics(butil::rapidjson::kArrayType);
    
    for (const auto& problem_ptr : problemList) {
        const auto& problem = *problem_ptr;
        butil::rapidjson::Value prob_obj(butil::rapidjson::kObjectType);
        
        // 添加基本字段
        prob_obj.AddMember("name", 
            butil::rapidjson::Value(problem.title().c_str(), allocator), allocator);
        
        // 从ID中解析contestId和index
        std::string id = problem.id();
        std::string contestId_str, index_str;
        
        // 解析逻辑：从末尾开始找第一个字母位置
        size_t split_pos = id.length();
        for (size_t i = id.length(); i > 0; i--) {
            if (std::isalpha(id[i-1])) {
                split_pos = i;
                break;
            }
        }
        
        if (split_pos > 0 && split_pos <= id.length()) {
            contestId_str = id.substr(0, split_pos - 1);
            index_str = id.substr(split_pos - 1);
        }
        
        if (!contestId_str.empty()) {
            prob_obj.AddMember("contestId", std::stoi(contestId_str), allocator);
        }
        if (!index_str.empty()) {
            prob_obj.AddMember("index", 
                butil::rapidjson::Value(index_str.c_str(), allocator), allocator);
        }
        
        prob_obj.AddMember("rating", problem.rating(), allocator);
        
        // 添加tags数组
        butil::rapidjson::Value tags_array(butil::rapidjson::kArrayType);
        for (int i = 0; i < problem.tag_size(); i++) {
            tags_array.PushBack(
                butil::rapidjson::Value(problem.tag(i).c_str(), allocator), allocator);
        }
        prob_obj.AddMember("tags", tags_array, allocator);
        
        // 添加analyzed_tags数组
        butil::rapidjson::Value analyzed_tags_array(butil::rapidjson::kArrayType);
        for (int i = 0; i < problem.analyzed_tag_size(); i++) {
            analyzed_tags_array.PushBack(
                butil::rapidjson::Value(problem.analyzed_tag(i).c_str(), allocator), allocator);
        }
        prob_obj.AddMember("analyzed_tags", analyzed_tags_array, allocator);
        
        problems.PushBack(prob_obj, allocator);
        
        // 添加统计信息
        butil::rapidjson::Value stat_obj(butil::rapidjson::kObjectType);
        if (!contestId_str.empty()) {
            stat_obj.AddMember("contestId", std::stoi(contestId_str), allocator);
        }
        if (!index_str.empty()) {
            stat_obj.AddMember("index", 
                butil::rapidjson::Value(index_str.c_str(), allocator), allocator);
        }
        stat_obj.AddMember("solvedCount", problem.solved(), allocator);
        problemStatistics.PushBack(stat_obj, allocator);
    }
    
    doc.AddMember("problems", problems, allocator);
    doc.AddMember("problemStatistics", problemStatistics, allocator);
    doc.AddMember("timestamp", static_cast<int64_t>(std::time(nullptr)), allocator);
    
    // 写入文件
    butil::rapidjson::StringBuffer buffer;
    butil::rapidjson::PrettyWriter<butil::rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);
    
    return utils::FileUtils::WriteFileContent(PROBLEMS_FILE_PATH, buffer.GetString());
}

}
