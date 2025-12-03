const { loadCache } = require('./analyzeProblems');

const cache = loadCache();
const testProblem = cache.problems.find(problem => 
    problem.contestId === 550 && problem.index === 'A'
);

if (testProblem) {
    console.log('找到题目:', testProblem.name);
    console.log('analyzed_tags:', JSON.stringify(testProblem.analyzed_tags));
    console.log('原始tags:', JSON.stringify(testProblem.tags));
} else {
    console.log('未找到CF550A题目');
}

// 统计已处理的题目数量
const processedCount = cache.problems.filter(p => 
    p.analyzed_tags && p.analyzed_tags.length > 0
).length;

console.log(`总题目数: ${cache.problems.length}`);
console.log(`已处理题目数: ${processedCount}`);
console.log(`未处理题目数: ${cache.problems.length - processedCount}`);