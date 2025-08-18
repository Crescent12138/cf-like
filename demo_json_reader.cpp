#include <iostream>
#include <vector>
#include "src/util/cfProblemJsonReader.h"

int main() {
    using namespace suggest;
    
    // 加载所有题目
    std::cout << "=== 加载所有题目 ===" << std::endl;
    auto all_problems = CfProblemJsonReader::LoadAllProblems();
    std::cout << "总共加载了 " << all_problems.size() << " 道题目" << std::endl;
    
    // 筛选Rating在800-2000之间的题目（过滤掉异常值）
    std::cout << "\n=== Rating 800-2000的题目 ===" << std::endl;
    auto normal_problems = CfProblemJsonReader::FilterByRating(all_problems, 800, 2000);
    std::cout << "找到 " << normal_problems.size() << " 道正常难度题目" << std::endl;
    
    // 从正常题目中显示前5道
    std::cout << "\n=== 前5道正常难度题目信息 ===" << std::endl;
    for (size_t i = 0; i < std::min(static_cast<size_t>(5), normal_problems.size()); ++i) {
        const auto& problem = normal_problems[i];
        std::cout << "题目 " << (i+1) << ": " << problem->title() 
                  << " (Rating: " << problem->rating() 
                  << ", Solved: " << problem->solved() << ")" << std::endl;
    }
    
    // 筛选包含特定标签的题目
    std::cout << "\n=== 包含'dp'或'greedy'标签的题目 ===" << std::endl;
    std::vector<std::string> target_tags = {"dp", "greedy"};
    auto dp_greedy_problems = CfProblemJsonReader::FilterByTags(normal_problems, target_tags);
    std::cout << "在正常难度题目中找到 " << dp_greedy_problems.size() << " 道题目包含目标标签" << std::endl;
    
    // 显示前3道DP/贪心题目
    for (size_t i = 0; i < std::min(static_cast<size_t>(3), dp_greedy_problems.size()); ++i) {
        const auto& problem = dp_greedy_problems[i];
        std::cout << "\n题目: " << problem->title() << " (Rating: " << problem->rating() << ")" << std::endl;
        std::cout << "  标签: ";
        for (int j = 0; j < problem->tag_size(); ++j) {
            std::cout << problem->tag(j);
            if (j < problem->tag_size() - 1) std::cout << ", ";
        }
        std::cout << std::endl;
        std::cout << "  URL: " << problem->url() << std::endl;
    }
    
    // 统计各难度段的题目数量
    std::cout << "\n=== 各难度段统计 ===" << std::endl;
    std::vector<std::pair<std::string, std::pair<int, int>>> difficulty_ranges = {
        {"新手", {800, 1000}},
        {"入门", {1000, 1200}},
        {"初级", {1200, 1400}},
        {"中级", {1400, 1600}},
        {"高级", {1600, 1800}},
        {"专家", {1800, 2000}}
    };
    
    for (const auto& range : difficulty_ranges) {
        auto range_problems = CfProblemJsonReader::FilterByRating(all_problems, range.second.first, range.second.second);
        std::cout << "  " << range.first << " (" << range.second.first << "-" << range.second.second << "): " 
                  << range_problems.size() << " 道题目" << std::endl;
    }
    
    // 检查缓存文件状态
    std::cout << "\n=== 缓存文件状态 ===" << std::endl;
    if (CfProblemJsonReader::IsCacheFileValid()) {
        long timestamp = CfProblemJsonReader::GetCacheTimestamp();
        std::cout << "缓存文件有效，时间戳: " << timestamp << std::endl;
    } else {
        std::cout << "缓存文件无效或不存在" << std::endl;
    }
    
    return 0;
}
