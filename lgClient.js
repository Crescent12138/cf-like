const request = require('request');
const cheerio = require('cheerio');

// 定义目标网址
const url = 'https://www.luogu.com.cn/problem/solution/CF433D';

// 配置请求选项
const options = {
    url: url,
    headers: {
        'Cookie': '__client_id=4b3c5e4a9e86a79868d4f2a82627644d8067d450; _uid=1093351; C3VK=8025c2',
        'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/140.0.0.0 Safari/537.36',
        'Accept': 'text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8',
        'Accept-Language': 'zh-CN,zh;q=0.9,en;q=0.8',
        'Referer': 'https://www.luogu.com.cn/'
    }
};

// 使用request模块发送GET请求
request(options, function (error, response, body) {
    if (!error && response.statusCode == 200) {
        console.log('请求成功，状态码:', response.statusCode);
        console.log('页面内容长度:', body.length);
        
        // 提取JSON数据
        const jsonMatch = body.match(/<script id="lentille-context" type="application\/json">(.*?)<\/script>/s);
        if (jsonMatch) {
            try {
                const jsonData = JSON.parse(jsonMatch[1]);
                const solutions = jsonData.data.solutions.result;
                
                console.log('\n=== 题解内容 ===');
                solutions.forEach((solution, index) => {
                    console.log(`\n题解 ${index + 1}:`);
                    console.log(`标题: ${solution.title}`);
                    console.log(`作者: ${solution.author.name}`);
                    console.log(`时间: ${new Date(solution.time * 1000).toLocaleString()}`);
                    console.log(`\n内容:`);
                    console.log('---开始---');
                    console.log(solution.content);
                    console.log('---结束---');
                    console.log('\n' + '='.repeat(80));
                });
                
            } catch (e) {
                console.error('JSON解析失败:', e);
                console.log('原始JSON数据:', jsonMatch[1].substring(0, 1000) + '...');
            }
        } else {
            console.log('未找到JSON数据');
            
            // 使用cheerio作为备选方案
            const $ = cheerio.load(body);
            
            // 获取页面标题
            const pageTitle = $('title').text();
            console.log('页面标题:', pageTitle);
            
            // 尝试提取可见的题解内容
            console.log('\n=== 尝试提取可见内容 ===');
            
            $('li').each(function(index) {
                const titleLink = $(this).find('h3 a');
                const author = $(this).find('small');
                
                if (titleLink.length > 0) {
                    console.log(`\n题解 ${index + 1}:`);
                    console.log(`标题: ${titleLink.text()}`);
                    console.log(`链接: ${titleLink.attr('href')}`);
                    console.log(`作者和时间: ${author.text()}`);
                }
            });
        }
        
    } else {
        console.error('请求失败');
        console.error('错误:', error);
        console.error('状态码:', response ? response.statusCode : 'N/A');
        
        // 即使失败也打印body内容，帮助调试
        if (body) {
            console.log('=== 错误响应内容 ===');
            console.log(body);
        }
    }
});