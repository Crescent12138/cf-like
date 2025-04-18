#include "constant/status.h"
#include "strategy/strategy.h"
#include <brpc/channel.h>
#include <butil/logging.h>
#include <json2pb/rapidjson.h>
#include "com/context.h"
#include <vector>
#include <random>   // 用于随机数生成
#include <numeric>  // 用于iota函数

// #define VALIDATE_PROBLEM_FIELD(problem, field, check_type, index) \
//     do { \
//         if ((!(problem).HasMember(field)) || (!(problem)[field].check_type())) { \
//             LOG(WARNING) << "Invalid " #field " in problem at index: " << index; \
//             continue; \
//         } \
//     } while(0)

bool ValidateProblemField(
    const butil::rapidjson::Value& problem,
    const char* field,
    bool (butil::rapidjson::Value::*checkFunc)() const,
    size_t index) 
{
    if (!problem.HasMember(field) || !(problem[field].*checkFunc)()) {
        //LOG(WARNING) << "Invalid " << field << " in problem at index: " << index;
        return false;
    }
    return true;
}

namespace suggest {
    base::Status RecallProcessor::Exec(Context *ctx) {
        //1. 请求codeforces cf 请求超时设为3s，后续异步处理该逻辑,存储在统一的异步词表中
        //2. 解析结果
        //3. 截断500
        //4. 随机若干个并输出
          //request codeforces
    
        brpc::ChannelOptions options;
        options.protocol = brpc::PROTOCOL_HTTP;  // or brpc::PROTOCOL_H2
        options.timeout_ms = 15000;
        options.connect_timeout_ms = 5000;
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
        
        // {
        //     butil::rapidjson::Document result;
        //     if(doc.HasMember("status") && doc["status"].IsString()){
        //         std::string status = doc["status"].GetString();
        //         if (status != std::string("OK")){
        //             LOG(ERROR) << "status :" << doc["status"].GetString() <<"\n";
        //             return base::Status::EARLY_RETURN;
        //         }
        //         if (doc.HasMember("result") && doc["result"].HasMember("problems") &&  doc["result"]["problems"].IsArray()){
        //             auto &problems = doc["result"]["problems"];
        //             if(problems.Size() > 0){
        //                 LOG(INFO) << "size :" <<problems.Size() <<"\n";
        //             }
        //             auto * feed = ctx->resp_->mutable_feedlist()->Add();
        //             // feed->mutable_title() = ;
        //             feed->set_title(problems[0]["name"].GetString());
                    
        //         }
        //     }
        // }
        {
            if (doc.HasMember("result") && doc["result"].IsObject()) {
                const auto& result = doc["result"];
                if (result.HasMember("problems") && result["problems"].IsArray()) {
                    const auto& problems = result["problems"];
                    const int problem_count = problems.Size();
                    
                    if (problem_count == 0) {
                        LOG(ERROR) << "Empty problems data";
                        return base::Status::OK;
                    }

                    LOG(INFO) << "Request1: " << ctx->query << "\n";

                    butil::rapidjson::Document query_doc;
                    int user_rating = 0;
                    if (query_doc.Parse(ctx->query.c_str()).HasParseError()) {
                        LOG(ERROR) << "Failed to parse query JSON";
                    }
                    else {
                        if (!query_doc.HasMember("rating") || !query_doc["rating"].IsInt()) {
                            LOG(ERROR) << "Missing or invalid rating in query";
                        }
                        else user_rating = query_doc["rating"].GetInt();
                    }
            
                    LOG(INFO) << "User rating: " << user_rating << "\n";

                    std::vector<size_t> valid_indices;
                    for (size_t i = 0; i < problem_count; ++i) {
                        const auto& problem = problems[i];

                        // 先检查是否是对象类型
                        if (!problem.IsObject()) {
                        LOG(WARNING) << "Problem at index " << i << " is not an object";
                        continue;
                        }

                        // 校验必须字段
                        if (!ValidateProblemField(problem, "name", &butil::rapidjson::Value::IsString, i) ||
                        !ValidateProblemField(problem, "contestId", &butil::rapidjson::Value::IsInt, i) ||
                        !ValidateProblemField(problem, "index", &butil::rapidjson::Value::IsString, i) ||
                        !ValidateProblemField(problem, "rating", &butil::rapidjson::Value::IsInt, i)) 
                            continue;  // 任意字段校验失败则跳过

                        int problem_rating = problem["rating"].GetInt();
                        if (problem_rating >= (user_rating - 300) && problem_rating <= (user_rating + 300)) {
                            valid_indices.push_back(i);
                        }
                    }

                    if (valid_indices.empty()) {
                        LOG(WARNING)<<"No problems found within rating range";
                        return base::Status::OK;
                    }
        
                    // 从配置获取随机数量
                    const int k = 5; 
                    const int select_count = std::min(k, static_cast<int>(valid_indices.size()));
        
                    // 生成随机索引
                    std::vector<size_t> indices(valid_indices.size());
                    std::iota(indices.begin(), indices.end(), 0);
                    
                    // 部分洗牌算法
                    std::random_device rd;
                    std::mt19937 rng(rd());
                    for (int i = 0; i < select_count; ++i) {
                        std::uniform_int_distribution<size_t> dist(i, valid_indices.size() - 1);
                        size_t j = dist(rng);
                        std::swap(indices[i], indices[j]);
                    }
        
                    // 填充结果
                    for (int i = 0; i < select_count; ++i) {
                        const auto& problem = problems[valid_indices[indices[i]]];
                        
                        const int contest_id = problem["contestId"].GetInt();
                        const std::string index = problem["index"].GetString();
                        const std::string full_id = std::to_string(contest_id) + index;
                        
                        auto* feed = ctx->resp_->mutable_feedlist()->Add();
                        feed->set_title(problem["name"].GetString());
                        feed->set_id(full_id);
                    }
                }
            }
        }
        
        return base::Status::OK;
    }
}