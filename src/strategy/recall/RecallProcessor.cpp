#include <brpc/channel.h>
#include <butil/logging.h>
#include <json2pb/rapidjson.h>

#include <numeric>  // 用于iota函数
#include <random>   // 用于随机数生成
#include <vector>

#include "com/context.h"
#include "constant/status.h"
#include "strategy/strategy.h"

// #define VALIDATE_PROBLEM_FIELD(problem, field, check_type, index) \
//     do { \
//         if ((!(problem).HasMember(field)) || (!(problem)[field].check_type())) { \
//             LOG(WARNING) << "Invalid " #field " in problem at index: " << index; \
//             continue; \
//         } \
//     } while(0)

bool ValidateProblemField(const butil::rapidjson::Value& problem, const char*        field,
                          bool (butil::rapidjson::Value::*checkFunc)() const, size_t index) {
    if (!problem.HasMember(field) || !(problem[field].*checkFunc)()) {
        // LOG(WARNING) << "Invalid " << field << " in problem at index: " << index;
        return false;
    }
    return true;
}

namespace suggest {
base::Status RecallProcessor::Exec(Context* ctx) {
    // 1. 请求codeforces cf 请求超时设为10s，后续异步处理该逻辑,存储在统一的异步词表中
    // 2. 解析结果
    // 3. 截断500
    // 4. 随机若干个并输出
    butil::rapidjson::Document query_doc;
    int                        user_rating = 0;
    if (query_doc.Parse(ctx->query.c_str()).HasParseError()) {
        LOG(ERROR) << "Failed to parse query JSON";
    } else {
        if (!query_doc.HasMember("rating") || !query_doc["rating"].IsInt()) {
            LOG(ERROR) << "Missing or invalid rating in query";
            user_rating = 1100;
        } else
            user_rating = query_doc["rating"].GetInt();
    }
    auto                problem_list = ctx->cf_problems->GetProblems();
    // LOG(INFO) << problem_list.size();
    std::vector<size_t> valid_indices;
    for (size_t i = 0; i < problem_list.size(); ++i) {
        const auto& problem        = problem_list[i];
        int         problem_rating = problem->rating();
        if (problem_rating >= (user_rating - 300) && problem_rating <= (user_rating + 300)) {
            valid_indices.push_back(i);
        }
    }

    if (valid_indices.empty()) {
        LOG(WARNING) << "No problems found within rating range";
        return base::Status::OK;
    }
    // 从配置获取随机数量
    const int k            = 5;
    const int select_count = std::min(k, static_cast<int>(valid_indices.size()));

    // 生成随机索引
    std::vector<size_t> indices(valid_indices.size());
    std::iota(indices.begin(), indices.end(), 0);

    // 部分洗牌算法
    std::random_device rd;
    std::mt19937       rng(rd());
    for (int i = 0; i < select_count; ++i) {
        std::uniform_int_distribution<size_t> dist(i, valid_indices.size() - 1);
        size_t                                j = dist(rng);
        std::swap(indices[i], indices[j]);
    }

    // 填充结果
    for (int i = 0; i < select_count; ++i) {
        const auto& problem = problem_list[valid_indices[indices[i]]];
        auto* feed = ctx->resp_->mutable_feedlist()->Add();
        feed->CopyFrom(*problem);
    }

    return base::Status::OK;
}
}  // namespace suggest