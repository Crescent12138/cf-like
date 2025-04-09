#pragma once
#include "client/cfClient.h"
#include "com/dataType.h"
#include "proto/feed.pb.h"
#include "util/jsonUtils.h"

namespace suggest {
class CfProblems : public DictData<std::string, Feed> {
public:
    CfProblems() {
        CfClient    client{};
        std::string status;
        int         code = 0;
        auto       &doc  = client.doc;
        if (doc.HasMember("status") && doc["status"].IsString()) {
            std::string status = doc["status"].GetString();
            if (status != std::string("OK")) {
                LOG(ERROR) << "status :" << doc["status"].GetString() << "\n";
                return;
            }
            if (doc.HasMember("result") && doc["result"].HasMember("problems") && doc["result"]["problems"].IsArray()) {
                auto &problems = doc["result"]["problems"];
                if (problems.Size() > 0) {
                    LOG(INFO) << "size :" << problems.Size() << "\n";
                }
                for (auto i = 0; i < problems.Size(); i++) {
                    Feed        feed;
                    std::string name = problems[i]["name"].GetString();
                    feed.set_title(name);
                }
                // auto * feed = ctx->resp_->mutable_feedlist()->Add();
                // // feed->mutable_title() = ;
                // feed->set_title(problems[0]["name"].GetString());
            }
        }
        // code = utils::get_rapidjson_string(client.doc, "status", status);
        // if(code != 0 || status != std::string("OK")){
        //     LOG(ERROR) << "code:" <<code <<";status:"<<status;
        //     return ;
        // } else {
        //     // auto  result = client.doc["result"];
        //     // code = utils::get_rapidjson_Object(client.doc, "result", result);
        //     if(code != 0){
        //         LOG(ERROR) << "code:" <<code;
        //         return ;
        //     }else{

        //     }
        // }
        // parse TODO
    }
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