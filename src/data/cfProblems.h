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
        // 删除本地文件强制重新获取
        utils::FileUtils::DeleteFile(CfProblems::PROBLEMS_FILE_PATH);
        load();
    }

    static std::shared_ptr<CfProblems> list_ptr;
};

static CfProblemHandler cfProblemHandler;
}  // namespace suggest