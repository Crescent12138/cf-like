#include "constant/status.h"
#include "strategy/strategy.h"
#include <brpc/channel.h>
#include <butil/logging.h>
#include <json2pb/rapidjson.h>
#include "com/context.h"
namespace suggest {
    base::Status RecallProcessor::Exec(Context *ctx){
        //1. 请求codeforces cf 请求超时设为3s，后续异步处理该逻辑,存储在统一的异步词表中
        //2. 解析结果
        //3. 截断500
        //4. 随机若干个并输出
          //request codeforces
    
        brpc::ChannelOptions options;
        options.protocol = brpc::PROTOCOL_HTTP;  // or brpc::PROTOCOL_H2
        options.timeout_ms = 4000;
        brpc::Channel codeforce_channel;
        if (codeforce_channel.Init("https://codeforces.com" /*any url*/, &options) != 0) {
            LOG(ERROR) << "Fail to initialize codeforce_channel";
            return base::Status::HTTP_ERROR;
        }
    
        brpc::Controller cntl;
        cntl.http_request().uri() = "https://codeforces.com/api/problemset.problems";  // 设置为待访问的URL
        codeforce_channel.CallMethod(NULL, &cntl, NULL, NULL, NULL/*done*/);
        if(cntl.ErrorCode() != 0){
            LOG(ERROR) << "error:" << cntl.ErrorCode() <<" text:"<<cntl.ErrorText()<<"\n";
            return base::Status::HTTP_ERROR;
        }
        butil::rapidjson::Document doc;
        if(doc.Parse(cntl.response_attachment().to_string().c_str()).HasParseError()){
            LOG(ERROR) << "error: parse codeforces error!!\n";
            return base::Status::EARLY_RETURN; 
        }
        //** 随便出两个题测试一下
        
        {
            butil::rapidjson::Document result;
            if(doc.HasMember("status") && doc["status"].IsString()){
                std::string status = doc["status"].GetString();
                if (status != std::string("OK")){
                    LOG(ERROR) << "status :" << doc["status"].GetString() <<"\n";
                    return base::Status::EARLY_RETURN;
                }
                if (doc.HasMember("result") && doc["result"].HasMember("problems") &&  doc["result"]["problems"].IsArray()){
                    auto &problems = doc["result"]["problems"];
                    if(problems.Size() > 0){
                        LOG(INFO) << "size :" <<problems.Size() <<"\n";
                    }
                    auto * feed = ctx->resp_->mutable_feedlist()->Add();
                    // feed->mutable_title() = ;
                    feed->set_title(problems[0]["name"].GetString());

                }
            }
        }
        
        return base::Status::OK;
    }
}