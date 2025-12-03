const fs = require('fs');

// 读取缓存文件
const cache = JSON.parse(fs.readFileSync('./cf_problems_cache.json', 'utf8'));

const totalProblems = cache.problems.length;
const needAnalysis = cache.problems.filter(p => !p.analyzed_tags || p.analyzed_tags.length === 0).length;
const hasAnalysis = totalProblems - needAnalysis;

console.log('=== CF题目缓存统计 ===');
console.log('总题目数:', totalProblems);
console.log('需要分析:', needAnalysis);
console.log('已有分析:', hasAnalysis);
console.log('分析进度:', Math.round(hasAnalysis / totalProblems * 100) + '%');

// 显示前5个需要分析的题目示例
const sampleProblems = cache.problems
    .filter(p => !p.analyzed_tags || p.analyzed_tags.length === 0)
    .slice(0, 5);

console.log('\n前5个需要分析的题目:');
sampleProblems.forEach((p, index) => {
    console.log(`${index + 1}. CF${p.contestId}${p.index} - ${p.name}`);
});