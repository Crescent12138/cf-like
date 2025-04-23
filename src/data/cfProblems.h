#pragma once
#include "client/cfClient.h"
#include "com/dataType.h"
#include "proto/feed.pb.h"
#include "util/jsonUtils.h"

namespace suggest {
class CfProblems : public DictData<std::string, Feed> {
public:
    CfProblems() {
        int bucket_count = 10000;
        int load_factor = 80;
        map_.init(bucket_count, load_factor);
        CfClient    client{};
        std::string status;
        int         code = 0;
        auto       &doc  = client.doc;
        if (doc.HasMember("status") && doc["status"].IsString()) {
            std::string status = doc["status"].GetString();
            if (status != std::string("OK")) {
                LOG(ERROR) << "status :" << doc["status"].GetString();
                return;
            }
            if (doc.HasMember("result") && doc["result"].HasMember("problems")) {
                auto &problems = doc["result"]["problems"];
                if (problems.Size() > 0) {
                    LOG(INFO) << "size :" << problems.Size();
                }
                for (auto i = 0; i < problems.Size(); i++) {
                    Feed        feed;
                    std::string name      ;
                    int         contestId ;
                    std::string index     ;
                    int         rating    ;
                    utils::get_rapidjon_string(problems[i], "name", name);
                    utils::get_rapidjon_int(problems[i], "contestId", contestId);
                    utils::get_rapidjon_string(problems[i], "index", index);
                    utils::get_rapidjon_int(problems[i], "rating", rating, false);
                    std::vector<std::string> tags;
                    utils::get_rapidjon_vector_string(problems[i], "tags", tags);
                    feed.set_id(std::to_string(contestId).append(index));
                    feed.set_title(name);
                    feed.set_level(rating);
                    for(auto &tag: tags){
                        feed.add_tag(tag);
                    }
                    const std::string problem_url =
                    "https://codeforces.com/problemset/problem/" + std::to_string(contestId) + "/" + index;
                    feed.set_url(problem_url);
                    map_[feed.id()] = feed;
                    problemList.emplace_back(std::make_shared<Feed>(feed));
                }
                if (map_.size() > 0) {
                    LOG(INFO) << "problems_map_size :" << map_.size();
                }else{
                    LOG(WARNING) << "problems_Map is empty!!!";
                }
            }else{
                LOG(WARNING) << "warning with cf-problem-result!!!";
            }
        }else{
            LOG(WARNING) << "warning with cf-problem";
        }
    }
    std::vector<std::shared_ptr<Feed>>& GetProblems(){
        return problemList;
    }
private:
std::vector<std::shared_ptr<Feed>> problemList;
};

class CfProblemHandler {
public:
    static void load() {
        std::shared_ptr<CfProblems> pk_ptr = std::make_shared<CfProblems>();
        list_ptr                           = pk_ptr;
    }

    static std::shared_ptr<CfProblems> list_ptr;
};

static CfProblemHandler     cfProblemHandler;
}  // namespace suggest