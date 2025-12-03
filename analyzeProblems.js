const fs = require('fs');
const request = require('request');
const cheerio = require('cheerio');
const { default:  axios } = require('axios');

// 配置
const CONFIG = {
    cacheFile: './cf_problems_cache.json',
    luoguBaseUrl: 'https://www.luogu.com.cn/problem/solution/',
    delay: 300, // 请求间隔，避免被限制
    maxRetries: 3,
    cookie: '__client_id=259c7e72e2e5e5a39e563a119577318ca0d9d33f; _uid=1093351; C3VK=ee8441',
    batchSave: 20 // 每处理多少个题目保存一次
};

// 算法相关关键词词典 (根据CF完整标签列表)
const ALGORITHM_KEYWORDS = {
    '2-sat': ['2-sat', '2sat', '布尔可满足性'],
    'binary search': ['二分', '二分查找', '二分答案', 'binary search'],
    'bitmasks': ['位运算', '状压', 'bitmask', '位掩码'],
    'brute force': ['暴力', '枚举', 'brute force'],
    'chinese remainder theorem': ['中国剩余定理', 'crt'],
    'combinatorics': ['组合数学', '组合', 'combinatorics'],
    'constructive algorithms': ['构造', '构造算法', 'constructive'],
    'data structures': ['数据结构', '线段树', '树状数组', '并查集', '堆', '栈', '队列'],
    'dfs and similar': ['dfs', '深度优先', '深度优先搜索', '回溯'],
    'divide and conquer': ['分治', '递归'],
    'dp': ['动态规划', 'dp', '状态转移', '记忆化'],
    'dsu': ['并查集', 'dsu', 'union find'],
    'expression parsing': ['表达式解析', '表达式求值'],
    'fft': ['快速傅里叶变换', 'fft', 'ntt'],
    'flows': ['网络流', '最大流', 'flow'],
    'games': ['博弈论', 'nim', '博弈'],
    'geometry': ['几何', '计算几何', '凸包', '线段相交'],
    'graph matchings': ['图匹配', '二分图匹配', '最大匹配'],
    'graphs': ['图论', '图', '最短路', 'dijkstra', 'floyd', 'spfa', '最小生成树', 'mst', '拓扑排序'],
    'greedy': ['贪心', 'greedy', '局部最优'],
    'hashing': ['哈希', '散列', 'hash'],
    'implementation': ['实现', '模拟', 'implementation'],
    'interactive': ['交互', '交互题'],
    'math': ['数学', '数论', 'math', 'gcd', 'lcm', '质数', '素数', '概率'],
    'matrices': ['矩阵', '矩阵运算'],
    'meet-in-the-middle': ['折半搜索', 'meet in the middle'],
    'number theory': ['数论', '欧几里得', '因式分解'],
    'probabilities': ['概率', '期望'],
    'schedules': ['调度', '任务调度'],
    'shortest paths': ['最短路'],
    'sortings': ['排序', 'sort', '归并排序', '快速排序'],
    'string suffix structures': ['后缀数组', '后缀树', '后缀自动机'],
    'strings': ['字符串', 'string', 'kmp', '字符串匹配', 'trie'],
    'ternary search': ['三分', '三分查找'],
    'trees': ['树', '二叉树', '树形dp'],
    'two pointers': ['双指针', '两指针']
};

// 读取缓存文件
function loadCache() {
    try {
        const data = fs.readFileSync(CONFIG.cacheFile, 'utf8');
        return JSON.parse(data);
    } catch (error) {
        console.error('读取缓存文件失败:', error);
        return null;
    }
}

// 保存缓存文件
function saveCache(data) {
    try {
        fs.writeFileSync(CONFIG.cacheFile, JSON.stringify(data, null, 2));
        console.log('缓存文件保存成功');
        return true;
    } catch (error) {
        console.error('保存缓存文件失败:', error);
        return false;
    }
}

// 延时函数
function delay(ms) {
    return new Promise(resolve => setTimeout(resolve, ms));
}

// 构造洛谷题解URL
function buildLuoguUrl(problem) {
    // 从contestId和index构造problemId
    const problemId = `CF${problem.contestId}${problem.index}`;
    return CONFIG.luoguBaseUrl + problemId;
}

// 获取题目ID
function getProblemId(problem) {
    return `${problem.contestId}${problem.index}`;
}

// 爬取题解内容
function  fetchSolution(url) {
    return new Promise((resolve, reject) => {
        const options = {
            url: url,
            headers: {
                'Cookie': CONFIG.cookie,
                'Accept': 'text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8',
                'Accept-Language': 'zh-CN,zh;q=0.9,en;q=0.8',
                'Referer': 'https://www.luogu.com.cn/'
            },
            timeout: 10000
        };

        request(options, (error, response, body) => {
            if (error) {
                reject(error);
                return;
            }

            if (response.statusCode !== 200) {
                reject(new Error(`HTTP ${response.statusCode}`));
                return;
            }

            try {
                // 提取JSON数据
            
                const jsonMatch = body.match(/<script id="lentille-context" type="application\/json">(.*?)<\/script>/s);
                
                
                if (jsonMatch) {
                    const jsonData = JSON.parse(jsonMatch[1]);
                    console.log(jsonData);
                    const solutions = jsonData.data.solutions.result || [];
                    
                    let allContent = '';
                    solutions.forEach(solution => {
                        if (solution.content) {
                            allContent += solution.content + '\n';
                        }
                    });
                    
                    resolve(allContent);
                } else {
                    resolve(''); // 没有找到内容
                }
            } catch (parseError) {
                reject(parseError);
            }
        });
    });
}

// 分析文本内容，提取算法标签
function analyzeContent(content) {
    const analyzedTags = new Set();
    const lowerContent = content.toLowerCase();
    
    // 遍历算法关键词字典
    for (const [tag, keywords] of Object.entries(ALGORITHM_KEYWORDS)) {
        for (const keyword of keywords) {
            if (lowerContent.includes(keyword.toLowerCase())) {
                analyzedTags.add(tag);
                break; // 找到一个关键词就足够了
            }
        }
    }
    
    return Array.from(analyzedTags);
}

// 处理单个题目
async function processProblem(problem, index, total) {
    const problemId = getProblemId(problem);
    console.log(`\n[${index + 1}/${total}] 处理题目: ${problemId} - ${problem.name}`);
    
    // 检查是否已有analyzed_tags
    if (problem.analyzed_tags && problem.analyzed_tags.length > 0) {
        console.log('  已有分析标签，跳过');
        return problem;
    }
    
    const url = buildLuoguUrl(problem);
    console.log(`  URL: ${url}`);
    
    let retries = 0;
    while (retries < CONFIG.maxRetries) {
        try {
            console.log(`  正在获取题解内容... (尝试 ${retries + 1}/${CONFIG.maxRetries})`);
            const content = await fetchSolution(url);
            
            if (!content || content.trim() === '') {
                console.log('  未找到题解内容');
                return problem;
            }
            
            console.log(`  获取到内容长度: ${content.length} 字符`);
            
            // 分析内容
            const analyzedTags = analyzeContent(content);
            console.log(`  分析出标签: ${analyzedTags.join(', ') || '无'}`);
            
            // 更新题目
            const updatedProblem = { ...problem };
            updatedProblem.analyzed_tags = analyzedTags;
            
            return updatedProblem;
            
        } catch (error) {
            retries++;
            console.log(`  错误: ${error.message}`);
            
            if (retries < CONFIG.maxRetries) {
                console.log(`  等待 ${CONFIG.delay}ms 后重试...`);
                await delay(CONFIG.delay);
            }
        }
    }
    
    console.log('  处理失败，保持原状');
    return problem;
}

// 主函数
async function main() {
    console.log('开始分析CF题目...');
    
    // 加载缓存
    const cache = loadCache();
    if (!cache || !cache.problems) {
        console.error('无法加载题目缓存');
        return;
    }
    
    console.log(`总共 ${cache.problems.length} 个题目`);
    
    // 筛选需要处理的题目 (所有无analyzed_tags的题目)
    const problemsToProcess = cache.problems
        .filter(problem => !problem.analyzed_tags || problem.analyzed_tags.length === 0)
        .sort((a, b) => a.contestId - b.contestId); // 按contestId排序，从早期题目开始
    
    console.log(`需要处理 ${problemsToProcess.length} 个题目`);
    
    if (problemsToProcess.length === 0) {
        console.log('所有题目都已分析完成');
        return;
    }
    
    // 处理每个题目，支持批量保存
    const updatedProblems = [...cache.problems]; // 复制原数组
    let processedCount = 0;
    let successCount = 0;
    
    for (let i = 0; i < updatedProblems.length; i++) {
        const problem = updatedProblems[i];
        
        // 找到需要处理的题目
        const shouldProcess = problemsToProcess.find(p => getProblemId(p) === getProblemId(problem));
        if (!shouldProcess) {
            continue;
        }
        
        const processedIndex = problemsToProcess.indexOf(shouldProcess);
        console.log(`\n[${processedIndex + 1}/${problemsToProcess.length}] 处理题目: ${getProblemId(problem)} - ${problem.name}`);
        
        try {
            const updatedProblem = await processProblem(problem, processedIndex, problemsToProcess.length);
            
            // 检查是否成功添加了标签
            if (updatedProblem.analyzed_tags && updatedProblem.analyzed_tags.length > 0) {
                successCount++;
                console.log(`  ✓ 成功分析，标签: ${updatedProblem.analyzed_tags.join(', ')}`);
            } else {
                console.log(`  - 未找到标签或题解`);
            }
            
            updatedProblems[i] = updatedProblem;
            processedCount++;
            
            // 批量保存
            if (processedCount % CONFIG.batchSave === 0) {
                console.log(`\n--- 批量保存进度 (${processedCount}/${problemsToProcess.length}) ---`);
                const tempCache = { ...cache, problems: updatedProblems };
                if (saveCache(tempCache)) {
                    console.log(`✓ 已保存 ${processedCount} 个题目的进度`);
                } else {
                    console.log('✗ 保存失败');
                }
            }
            
            // 请求间隔
            if (processedIndex < problemsToProcess.length - 1) {
                console.log(`  等待 ${CONFIG.delay}ms...`);
                await delay(CONFIG.delay);
            }
            
        } catch (error) {
            console.log(`  ✗ 处理失败: ${error.message}`);
            processedCount++;
        }
    }
    
    // 最终保存
    console.log(`\n=== 处理完成统计 ===`);
    console.log(`总处理: ${processedCount} 个题目`);
    console.log(`成功分析: ${successCount} 个题目`);
    console.log(`成功率: ${processedCount > 0 ? Math.round(successCount / processedCount * 100) : 0}%`);
    
    const finalCache = { ...cache, problems: updatedProblems };
    
    if (saveCache(finalCache)) {
        console.log('\n分析完成！缓存文件已更新');
    } else {
        console.log('\n分析完成，但保存失败');
    }
}

// 运行主函数
if (require.main === module) {
    main().catch(console.error);
}

module.exports = {
    loadCache,
    saveCache,
    buildLuoguUrl,
    fetchSolution,
    analyzeContent,
    processProblem,
    getProblemId
};