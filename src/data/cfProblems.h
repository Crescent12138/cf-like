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
            if (CHECK_JSON_FIELD(doc, "result", Object) && CHECK_JSON_FIELD(doc["result"], "problems", Array)) {
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