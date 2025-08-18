#include "cfProblemJsonReader.h"
#include <butil/logging.h>
#include <json2pb/rapidjson.h>
#include <sys/stat.h>
#include "util/jsonUtils.h"

namespace suggest {

const std::string CfProblemJsonReader::DEFAULT_CACHE_FILE = "cf_problems_cache.json";

std::vector<std::shared_ptr<Feed>> CfProblemJsonReader::LoadAllProblems() {
    return LoadProblemsFromFile(DEFAULT_CACHE_FILE);
}

std::vector<std::shared_ptr<Feed>> CfProblemJsonReader::LoadProblemsFromFile(const std::string& filepath) {
    std::vector<std::shared_ptr<Feed>> result;
    
    std::ifstream file(filepath);
    if (!file.is_open()) {
        LOG(WARNING) << "Cannot open file: " << filepath;
        return result;
    }
    
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();
    
    if (content.empty()) {
        LOG(WARNING) << "File is empty: " << filepath;
        return result;
    }
    
    butil::rapidjson::Document doc;
    if (doc.Parse(content.c_str()).HasParseError()) {
        LOG(ERROR) << "Failed to parse JSON file: " << filepath;
        return result;
    }
    
    if (!doc.HasMember("problems") || !doc["problems"].IsArray()) {
        LOG(ERROR) << "Invalid JSON format, missing 'problems' array in: " << filepath;
        return result;
    }
    
    // 解析solved count统计信息
    std::map<std::string, int> problem_solved;
    if (doc.HasMember("problemStatistics") && doc["problemStatistics"].IsArray()) {
        auto& statistics_array = doc["problemStatistics"];
        for (size_t i = 0; i < statistics_array.Size(); ++i) {
            const auto& stat = statistics_array[i];
            
            int contestId, solved = 0;
            std::string index;
            
            if (utils::get_rapidjon_int(stat, (char*)"contestId", contestId) &&
                utils::get_rapidjon_string(stat, (char*)"index", index)) {
                utils::get_rapidjon_int(stat, (char*)"solvedCount", solved, false);
                std::string problem_id = std::to_string(contestId) + index;
                problem_solved[problem_id] = solved;
            }
        }
    }
    
    // 解析题目信息
    auto& problems_array = doc["problems"];
    for (size_t i = 0; i < problems_array.Size(); ++i) {
        const auto& json_problem = problems_array[i];
        
        auto feed = std::make_shared<Feed>();
        if (ParseProblemFromJson(json_problem, *feed, problem_solved)) {
            result.push_back(feed);
        }
    }
    
    LOG(INFO) << "Loaded " << result.size() << " problems from " << filepath;
    return result;
}

bool CfProblemJsonReader::ParseProblemFromJson(
    const butil::rapidjson::Value& json_problem, 
    Feed& feed,
    const std::map<std::string, int>& problem_solved) {
    
    std::string name, index;
    int contestId = 0, rating = 0;
    
    // 解析基本字段
    if (!utils::get_rapidjon_string(json_problem, (char*)"name", name)) {
        return false;
    }
    if (!utils::get_rapidjon_int(json_problem, (char*)"contestId", contestId)) {
        return false;
    }
    if (!utils::get_rapidjon_string(json_problem, (char*)"index", index)) {
        return false;
    }
    
    // rating是可选字段
    utils::get_rapidjon_int(json_problem, (char*)"rating", rating, false);
    
    // 解析tags数组
    std::vector<std::string> tags;
    if (json_problem.HasMember("tags") && json_problem["tags"].IsArray()) {
        utils::get_rapidjon_vector_string(json_problem, (char*)"tags", tags);
    }
    
    // 设置Feed对象的字段
    std::string problem_id = std::to_string(contestId) + index;
    feed.set_id(problem_id);
    feed.set_title(name);
    feed.set_rating(rating);
    
    // 设置solved count
    auto it = problem_solved.find(problem_id);
    if (it != problem_solved.end()) {
        feed.set_solved(it->second);
    } else {
        feed.set_solved(0);
    }
    
    for (const auto& tag : tags) {
        feed.add_tag(tag);
    }
    
    // 构造URL
    const std::string problem_url = 
        "https://codeforces.com/problemset/problem/" + std::to_string(contestId) + "/" + index;
    feed.set_url(problem_url);
    
    return true;
}

std::vector<std::shared_ptr<Feed>> CfProblemJsonReader::FilterByRating(
    const std::vector<std::shared_ptr<Feed>>& problems, 
    int min_rating, 
    int max_rating) {
    
    std::vector<std::shared_ptr<Feed>> result;
    
    for (const auto& problem : problems) {
        if (problem->rating() >= min_rating && problem->rating() <= max_rating) {
            result.push_back(problem);
        }
    }
    
    return result;
}

std::vector<std::shared_ptr<Feed>> CfProblemJsonReader::FilterByTags(
    const std::vector<std::shared_ptr<Feed>>& problems, 
    const std::vector<std::string>& target_tags) {
    
    std::vector<std::shared_ptr<Feed>> result;
    
    for (const auto& problem : problems) {
        bool has_target_tag = false;
        
        for (int i = 0; i < problem->tag_size() && !has_target_tag; ++i) {
            const std::string& problem_tag = problem->tag(i);
            for (const std::string& target_tag : target_tags) {
                if (problem_tag == target_tag) {
                    has_target_tag = true;
                    break;
                }
            }
        }
        
        if (has_target_tag) {
            result.push_back(problem);
        }
    }
    
    return result;
}

bool CfProblemJsonReader::IsCacheFileValid() {
    struct stat file_stat;
    if (stat(DEFAULT_CACHE_FILE.c_str(), &file_stat) == 0) {
        return file_stat.st_size > 0;  // 文件存在且非空
    }
    return false;
}

long CfProblemJsonReader::GetCacheTimestamp() {
    std::ifstream file(DEFAULT_CACHE_FILE);
    if (!file.is_open()) {
        return 0;
    }
    
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();
    
    butil::rapidjson::Document doc;
    if (doc.Parse(content.c_str()).HasParseError()) {
        return 0;
    }
    
    if (doc.HasMember("timestamp") && doc["timestamp"].IsInt64()) {
        return doc["timestamp"].GetInt64();
    }
    
    return 0;
}

}  // namespace suggest