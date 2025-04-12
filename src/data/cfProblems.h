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
        // map_.insert(10, "hello");
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
            if (doc.HasMember("result") && doc["result"].HasMember("problems") && doc["result"]["problems"].IsArray()) {
                auto &problems = doc["result"]["problems"];
                if (problems.Size() > 0) {
                    LOG(INFO) << "size :" << problems.Size();
                }
                for (auto i = 0; i < problems.Size(); i++) {
                    Feed        feed;
                    std::string name      = problems[i].HasMember("name") ? problems[i]["name"].GetString() : "";
                    int         contestId = problems[i].HasMember("contestId") ? problems[i]["contestId"].GetInt() : 0;
                    std::string index     = problems[i].HasMember("index") ? problems[i]["index"].GetString() : "";
                    int         rating    = problems[i].HasMember("rating") ? problems[i]["rating"].GetInt() : 0;
                    // std::string type = problems[i]["type"].GetString();
                    std::vector<std::string> tags;
                    if (problems[i].HasMember("tags")) {
                        auto &ori_tags = problems[i]["tags"];
                        tags.reserve(ori_tags.Size());
                        for (int j = 0; j < ori_tags.Size(); j++) {
                            tags.emplace_back(ori_tags[j].GetString());
                        }
                    }

                    feed.set_id(std::to_string(contestId).append(index));
                    feed.set_title(name);
                    feed.set_level(rating);
                    for(auto &tag: tags){
                        feed.add_tag(tag);
                    }
                    // map_.emplace(feed.id(),feed);
                    map_[feed.id()] = feed;
                    problemList.emplace_back(std::make_shared<Feed>(feed));
                }
                if (map_.size() > 0) {
                    LOG(INFO) << "problems_map_size :" << map_.size();
                }
            }
        }
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
std::shared_ptr<CfProblems> CfProblemHandler::list_ptr = nullptr;
}  // namespace suggest