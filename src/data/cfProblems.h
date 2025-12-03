#pragma once
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "client/cfClient.h"
#include "com/dataType.h"
#include "proto/feed.pb.h"
#include "util/jsonUtils.h"
#include "util/jsonFileUtils.h"

namespace suggest {
class CfProblems : public DictData<std::string, Feed> {
public:
    CfProblems() {
        int bucket_count = 10000;
        int load_factor = 80;
        map_.init(bucket_count, load_factor);
        
        // 先尝试从本地文件加载
        if (LoadFromLocalFile()) {
            LOG(INFO) << "Successfully loaded problems from local file";
            return;
        }
        
        // 如果本地文件不存在或加载失败，则从CF获取
        LOG(INFO) << "Loading problems from Codeforces API";
        LoadFromCodeforces();
    }

    std::vector<std::shared_ptr<Feed>>& GetProblems(){
        return problemList;
    }

    static const std::string PROBLEMS_FILE_PATH;

private:
    std::vector<std::shared_ptr<Feed>> problemList;
    
    bool LoadFromLocalFile();
    void LoadFromCodeforces();
    bool SaveToLocalFile();
    void ParseProblemsFromJson(const butil::rapidjson::Value& problems, 
                              const std::map<std::string, int>& problem_solved);
};

class CfProblemHandler {
public:
    static void load() {
        std::shared_ptr<CfProblems> pk_ptr = std::make_shared<CfProblems>();
        list_ptr = pk_ptr;
    }
    
    // 强制刷新数据（从CF重新获取并保存到本地）
    static void forceRefresh() {
        LOG(INFO) << "Force refreshing problems from Codeforces";
        
        // 1. 备份 analyzed_tags 到内存
        std::map<std::string, std::vector<std::string>> analyzed_tags_backup;
        if (utils::FileUtils::FileExists(CfProblems::PROBLEMS_FILE_PATH)) {
            std::string json_str;
            if (!utils::FileUtils::ReadFileContent(CfProblems::PROBLEMS_FILE_PATH, json_str)) {
                LOG(WARNING) << "Failed to read cache file for backup";
            } else {
                butil::rapidjson::Document doc;
                doc.Parse(json_str.c_str());
            
            if (!doc.HasParseError() && doc.IsObject() && doc.HasMember("problems")) {
                const auto& problems = doc["problems"];
                for (auto it = problems.Begin(); it != problems.End(); ++it) {
                    if (!it->HasMember("contestId") || !it->HasMember("index") || 
                        !it->HasMember("analyzed_tags")) {
                        continue;
                    }
                    
                    const auto& analyzed_tags = (*it)["analyzed_tags"];
                    if (!analyzed_tags.IsArray() || analyzed_tags.Empty()) {
                        continue;
                    }
                    
                    // 构造 key: contestId-index
                    std::string key = std::to_string((*it)["contestId"].GetInt()) + "-" + 
                                    std::string((*it)["index"].GetString());
                    
                    // 提取 analyzed_tags
                    std::vector<std::string> tags;
                    for (auto tag_it = analyzed_tags.Begin(); tag_it != analyzed_tags.End(); ++tag_it) {
                        if (tag_it->IsString()) {
                            tags.push_back(tag_it->GetString());
                        }
                    }
                    
                    if (!tags.empty()) {
                        analyzed_tags_backup[key] = tags;
                    }
                }
                LOG(INFO) << "Backed up analyzed_tags for " << analyzed_tags_backup.size() << " problems";
            }
            }
        }
        
        // 2. 删除本地文件强制重新获取
        utils::FileUtils::DeleteFile(CfProblems::PROBLEMS_FILE_PATH);
        
        // 3. 重新加载
        load();
        
        // 4. 恢复 analyzed_tags
        if (!analyzed_tags_backup.empty() && utils::FileUtils::FileExists(CfProblems::PROBLEMS_FILE_PATH)) {
            std::string json_str;
            if (!utils::FileUtils::ReadFileContent(CfProblems::PROBLEMS_FILE_PATH, json_str)) {
                LOG(ERROR) << "Failed to read cache file for restoration";
                return;
            }
            butil::rapidjson::Document doc;
            doc.Parse(json_str.c_str());
            
            if (!doc.HasParseError() && doc.IsObject() && doc.HasMember("problems")) {
                auto& allocator = doc.GetAllocator();
                auto& problems = doc["problems"];
                int restored_count = 0;
                
                for (auto it = problems.Begin(); it != problems.End(); ++it) {
                    if (!it->HasMember("contestId") || !it->HasMember("index")) {
                        continue;
                    }
                    
                    std::string key = std::to_string((*it)["contestId"].GetInt()) + "-" + 
                                    std::string((*it)["index"].GetString());
                    
                    auto backup_it = analyzed_tags_backup.find(key);
                    if (backup_it != analyzed_tags_backup.end()) {
                        // 恢复 analyzed_tags
                        butil::rapidjson::Value tags_array(butil::rapidjson::kArrayType);
                        for (const auto& tag : backup_it->second) {
                            butil::rapidjson::Value tag_val(tag.c_str(), allocator);
                            tags_array.PushBack(tag_val, allocator);
                        }
                        
                        if (it->HasMember("analyzed_tags")) {
                            (*it)["analyzed_tags"] = tags_array;
                        } else {
                            it->AddMember("analyzed_tags", tags_array, allocator);
                        }
                        restored_count++;
                    }
                }
                
                // 保存恢复后的文件
                butil::rapidjson::StringBuffer buffer;
                butil::rapidjson::PrettyWriter<butil::rapidjson::StringBuffer> writer(buffer);
                doc.Accept(writer);
                if (!utils::FileUtils::WriteFileContent(CfProblems::PROBLEMS_FILE_PATH, buffer.GetString())) {
                    LOG(ERROR) << "Failed to write restored cache file";
                } else {
                    LOG(INFO) << "Restored analyzed_tags for " << restored_count << " problems";
                }
            }
        }
    }

    static std::shared_ptr<CfProblems> list_ptr;
};

static CfProblemHandler cfProblemHandler;
}  // namespace suggest