#pragma once

#include <vector>
#include <string>
#include <memory>
#include <map>
#include <fstream>
#include <iostream>
#include <json2pb/rapidjson.h>
#include "proto/feed.pb.h"

namespace suggest {

/**
 * JSON文件读取工具类，用于从本地缓存的JSON文件中读取CF题目信息
 * 提供灵活的题目筛选和查询功能
 */
class CfProblemJsonReader {
public:
    static const std::string DEFAULT_CACHE_FILE;
    
    /**
     * 从默认缓存文件加载所有题目
     * @return 题目列表
     */
    static std::vector<std::shared_ptr<Feed>> LoadAllProblems();
    
    /**
     * 从指定文件加载题目
     * @param filepath JSON文件路径
     * @return 题目列表
     */
    static std::vector<std::shared_ptr<Feed>> LoadProblemsFromFile(const std::string& filepath);
    
    /**
     * 根据rating范围筛选题目
     * @param problems 题目列表
     * @param min_rating 最小rating
     * @param max_rating 最大rating
     * @return 筛选后的题目列表
     */
    static std::vector<std::shared_ptr<Feed>> FilterByRating(
        const std::vector<std::shared_ptr<Feed>>& problems, 
        int min_rating, 
        int max_rating);
    
    /**
     * 根据标签筛选题目
     * @param problems 题目列表
     * @param target_tags 目标标签列表
     * @return 包含任一目标标签的题目列表
     */
    static std::vector<std::shared_ptr<Feed>> FilterByTags(
        const std::vector<std::shared_ptr<Feed>>& problems, 
        const std::vector<std::string>& target_tags);
    
    /**
     * 检查缓存文件是否有效
     * @return 文件存在且非空则返回true
     */
    static bool IsCacheFileValid();
    
    /**
     * 获取缓存文件的时间戳
     * @return 时间戳，失败返回0
     */
    static long GetCacheTimestamp();

private:
    /**
     * 从JSON对象解析单个题目
     * @param json_problem JSON题目对象
     * @param feed 输出的Feed对象
     * @param problem_solved solved count映射
     * @return 解析成功返回true
     */
    static bool ParseProblemFromJson(
        const butil::rapidjson::Value& json_problem, 
        Feed& feed,
        const std::map<std::string, int>& problem_solved);
};

}  // namespace suggest