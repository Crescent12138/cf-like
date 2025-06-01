#include <brpc/channel.h>
#include <butil/logging.h>
#include <json2pb/rapidjson.h>

#include <numeric>  // 用于iota函数
#include <random>   // 用于随机数生成
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <utility>

#include "com/context.h"
#include "constant/status.h"
#include "strategy/strategy.h"
#include "data/cfProblems.h"

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
    int                        user_rating = 1100;
    if (query_doc.Parse(ctx->query.c_str()).HasParseError()) {
        LOG(ERROR) << "Failed to parse query JSON";
    } else {
        if (!query_doc.HasMember("rating") || !query_doc["rating"].IsInt()) {
            LOG(ERROR) << "Missing or invalid rating in query";
            user_rating = 1100;
        } else
            user_rating = query_doc["rating"].GetInt();
    }
    user_rating = user_rating/100*100; // 将rating归一化到100的倍数
    auto                problem_list = ctx->cf_problems->GetProblems();

    std::vector<size_t> candidates;
    for (size_t i=0;i<problem_list.size();i++) {
        const auto& problem = problem_list[i];
        if (problem->rating() >= (user_rating - 300) && 
            problem->rating() <= (user_rating + 300)) {
            candidates.push_back(i);
        }
    }

    // 分组： 按tag分类题目
    std::unordered_map<std::string, std::vector<size_t>> tag_groups;
    for (const auto& index : candidates) {
        const auto& problem = problem_list[index];
        for (int i=0;i<problem->tag_size();i++) {
            const auto& tag = problem->tag(i);
            tag_groups[tag].push_back(index);
        }
    }

    // 准备三个热度分组：高/中/低
    std::vector<size_t> high_not;   // 前25%
    std::vector<size_t> mid_not;    // 中间50%
    std::vector<size_t> low_not;    // 后25%

    for (auto& [tag, indices] : tag_groups) {
        // 跳过小于4题的tag组（无法划分三档）
        if (indices.size() < 4) continue;

        // 按solved降序排序（解决人数越多越前）
        std::sort(indices.begin(), indices.end(), 
            [&](size_t a, size_t b) { 
                return problem_list[a]->solved() > problem_list[b]->solved(); 
            });

        // 计算分档位置
        size_t total = indices.size();
        size_t high_end = total / 4;           // 前25%位置
        size_t mid_end = high_end + total / 2;  // 中间50%结束位置
        size_t low_start = total - high_end;    // 后25%起始位置

        // 填充到三档集合
        high_not.insert(high_not.end(), indices.begin(), indices.begin() + high_end);
        mid_not.insert(mid_not.end(), indices.begin() + high_end, indices.begin() + mid_end);
        low_not.insert(low_not.end(), indices.begin() + low_start, indices.end());
    }

    // 选择题目
    std::unordered_map<std::string, bool> used_tags; // 记录已用tag
    std::vector<size_t> selected_problems;

    auto select_by_not = [&](auto& group, int count) -> bool {
        std::vector<size_t> temp;
        std::sample(
            group.begin(), group.end(), std::back_inserter(temp), count,
            std::mt19937{std::random_device{}()}
        );

        int cnt = 0;
        for (size_t index : temp) {
            const auto& prob = problem_list[index];
            for (const auto& tag : prob->tag()) {
                // 遇到新tag则使用该题
                if (!used_tags[tag]) {
                    used_tags[tag] = true; // 标记为已用
                    selected_problems.push_back(index);
                    cnt++;break;
                }
            }
        }
        return (cnt == count);
    };

    if (select_by_not(high_not, 1) &&
        select_by_not(mid_not, 2) &&
        select_by_not(low_not, 1)) {
        // 成功选择了每个热度组的题目共计4题
    } 
    else {
        // 如果选不足4题，随机补足
        // 移除重复题目和已选tag
        std::vector<size_t> remaining;
        for (size_t index : candidates) {
            const auto& prob = problem_list[index];
            bool tag_used = false;
            for (const auto& tag : prob->tag()) {
                if (used_tags[tag]) {
                    tag_used = true;
                    break;
                }
            }
            if (!tag_used && std::find(selected_problems.begin(), selected_problems.end(), index) == selected_problems.end()) {
                remaining.push_back(index);
            }
        }

        int need_count = 4 - selected_problems.size();
        if (need_count > 0 && remaining.size() >= need_count) {
            std::sample(
                remaining.begin(), remaining.end(),
                std::back_inserter(selected_problems), need_count,
                std::mt19937{std::random_device{}()}
            );
        } else {
            LOG(WARNING) << "Not enough unique problems to select from";
        }
    }

    for (size_t index : selected_problems) {
        const auto& problem = problem_list[index];
        auto* feed = ctx->resp_->mutable_feedlist()->Add();
        feed->CopyFrom(*problem);
    }

    return base::Status::OK;
}
}  // namespace suggest