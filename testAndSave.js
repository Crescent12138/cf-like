const { loadCache, saveCache, buildLuoguUrl, fetchSolution, analyzeContent, getProblemId } = require('./analyzeProblems');

async function testAndSave() {
    console.log('=== 测试并保存单个题目 ===');
    
    const cache = loadCache();
    if (!cache || !cache.problems) {
        console.error('无法加载题目缓存');
        return;
    }
    
    // 找一个特定题目进行测试
    const testProblem = cache.problems.find(problem => 
        problem.contestId === 550 && problem.index === 'A'
    );
    
    if (!testProblem) {
        console.log('未找到测试题目 CF550A');
        return;
    }
    
    console.log(`测试题目: ${getProblemId(testProblem)} - ${testProblem.name}`);
    console.log(`当前analyzed_tags: ${JSON.stringify(testProblem.analyzed_tags)}`);
    
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
        
        // 分析内容
        const analyzedTags = analyzeContent(content);
        console.log(`分析出的标签: ${JSON.stringify(analyzedTags)}`);
        
        // 更新题目
        testProblem.analyzed_tags = analyzedTags;
        
        // 保存缓存
        console.log('正在保存缓存...');
        if (saveCache(cache)) {
            console.log('保存成功！');
            
            // 验证保存结果
            const newCache = loadCache();
            const updatedProblem = newCache.problems.find(p => 
                p.contestId === testProblem.contestId && p.index === testProblem.index
            );
            console.log(`验证结果: ${JSON.stringify(updatedProblem.analyzed_tags)}`);
        } else {
            console.log('保存失败！');
        }
        
    } catch (error) {
        console.error('测试失败:', error);
    }
}

if (require.main === module) {
    testAndSave().catch(console.error);
}