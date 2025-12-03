const { loadCache, buildLuoguUrl, fetchSolution, analyzeContent, getProblemId } = require('./analyzeProblems');

// 测试单个题目
async function testSingleProblem() {
    console.log('=== 测试单个题目分析 ===');
    
    // 加载缓存找一个例子
    const cache = loadCache();
    if (!cache || !cache.problems) {
        console.error('无法加载题目缓存');
        return;
    }
    
    // 找一个较早的题目进行测试 (CF550A)
    let testProblem = cache.problems.find(problem => 
        problem.contestId === 550 && problem.index === 'A'
    );
    
    // 如果找不到，找第一个contestId小于1000的题目
    if (!testProblem) {
        testProblem = cache.problems.find(problem => 
            problem.contestId < 1000 && (!problem.analyzed_tags || problem.analyzed_tags.length === 0)
        );
    }
    
    // 如果还找不到，找任意一个
    if (!testProblem) {
        testProblem = cache.problems.find(problem => 
            !problem.analyzed_tags || problem.analyzed_tags.length === 0
        );
    }
    
    if (!testProblem) {
        console.log('没有找到需要分析的题目');
        return;
    }
    
    console.log(`测试题目: ${getProblemId(testProblem)} - ${testProblem.name}`);
    console.log(`原始标签: ${testProblem.tags ? testProblem.tags.join(', ') : '无'}`);
    
    const url = buildLuoguUrl(testProblem);
    console.log(`URL: ${url}`);
    
    try {
        console.log('正在获取题解内容...');
        const content = await fetchSolution(url);
        
        if (!content) {
            console.log('未找到题解内容');
            return;
        }
        
        console.log(`内容长度: ${content.length} 字符`);
        console.log(`内容预览: ${content.substring(0, 200)}...`);
        
        console.log('\n正在分析内容...');
        const analyzedTags = analyzeContent(content);
        
        console.log(`分析出的标签: ${analyzedTags.join(', ') || '无'}`);
        
        return { problem: testProblem, content, analyzedTags };
        
    } catch (error) {
        console.error('测试失败:', error);
    }
}

// 运行测试
if (require.main === module) {
    testSingleProblem().catch(console.error);
}

module.exports = { testSingleProblem };