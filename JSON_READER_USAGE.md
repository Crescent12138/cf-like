# CfProblemJsonReader 使用说明

`CfProblemJsonReader` 是一个用于从本地JSON缓存文件中解析和查询Codeforces题目信息的工具类。

## 主要功能

### 1. 加载题目数据
```cpp
// 从默认缓存文件加载所有题目
auto all_problems = CfProblemJsonReader::LoadAllProblems();

// 从指定文件加载题目
auto problems = CfProblemJsonReader::LoadProblemsFromFile("custom_cache.json");
```

### 2. 按难度筛选
```cpp
// 筛选Rating在1400-1600之间的题目
auto medium_problems = CfProblemJsonReader::FilterByRating(all_problems, 1400, 1600);
```

### 3. 按标签筛选
```cpp
// 筛选包含DP或贪心标签的题目
std::vector<std::string> target_tags = {"dp", "greedy"};
auto dp_greedy_problems = CfProblemJsonReader::FilterByTags(all_problems, target_tags);
```

### 4. 缓存状态检查
```cpp
// 检查缓存文件是否有效
if (CfProblemJsonReader::IsCacheFileValid()) {
    long timestamp = CfProblemJsonReader::GetCacheTimestamp();
    std::cout << "缓存时间戳: " << timestamp << std::endl;
}
```

## 使用示例

参考 `demo_json_reader.cpp` 文件中的完整示例。

## 编译和运行

```bash
# 编译demo程序
bazel build //:demo_json_reader

# 运行demo
./bazel-bin/demo_json_reader
```

## Feed对象属性

每个题目返回为 `Feed` 对象，包含以下主要属性：
- `id()`: 题目ID（contestId + index）
- `title()`: 题目标题
- `rating()`: 题目难度评级
- `solved()`: 解决人数
- `tag(i)`: 第i个标签
- `tag_size()`: 标签总数
- `url()`: 题目链接

## 性能特点

- **快速加载**: 从本地JSON文件加载，避免重复API调用
- **灵活筛选**: 支持多种筛选条件组合
- **内存高效**: 使用shared_ptr管理内存
- **线程安全**: 所有方法都是静态方法，可并发使用

## 注意事项

1. 确保 `cf_problems_cache.json` 文件存在且有效
2. Rating值可能包含异常值（如 2084058925），需要根据实际需求过滤
3. 标签筛选是"或"逻辑，只要包含任一目标标签即会匹配
