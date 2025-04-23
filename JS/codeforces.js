// ==UserScript==
// @name         Codeforces Rating & Problems
// @namespace    http://tampermonkey.net/
// @version      1.2
// @description  显示Rating并展示推荐题目列表
// @author       YourName
// @match        https://codeforces.com/*
// @grant        GM_xmlhttpRequest
// @grant        GM_addStyle
// ==/UserScript==

(function() {
    'use strict';

    // 添加自定义样式
    GM_addStyle(`
        #ratingButton {
            position: fixed;
            top: 10px;
            right: 20px;
            z-index: 9999;
            padding: 8px 16px;
            background: #4CAF50;
            color: white;
            border: none;
            border-radius: 4px;
            cursor: pointer;
            font-size: 14px;
            transition: all 0.3s;
        }
        #ratingButton:hover {
            background: #45a049;
        }
        .rating-status {
            position: fixed;
            top: 40px;
            right: 20px;
            font-size: 12px;
            max-width: 200px;
            text-align: right;
        }
        .problem-panel {
            position: fixed;
            top: 70px;
            right: 20px;
            width: 320px;
            max-height: 60vh;
            background: white;
            box-shadow: 0 4px 12px rgba(0,0,0,0.15);
            border-radius: 8px;
            padding: 15px;
            z-index: 9998;
            overflow-y: auto;
            display: none;
        }
        .problem-item {
            padding: 12px;
            margin: 8px 0;
            background: #f8f9fa;
            border-radius: 6px;
            transition: transform 0.2s;
            display: flex;
            justify-content: space-between;
            align-items: center;
        }
        .problem-item:hover {
            transform: translateX(5px);
            box-shadow: 0 2px 8px rgba(0,0,0,0.1);
        }
        .problem-title {
            font-weight: 500;
            margin-bottom: 4px;
        }
        .problem-rating {
            font-size: 12px;
            font-weight: 600;
        }
        .jump-button {
            padding: 6px 12px;
            background: #2196F3;
            color: white;
            border: none;
            border-radius: 4px;
            cursor: pointer;
            transition: background 0.3s;
        }
        .jump-button:hover {
            background: #1976D2;
        }
    `);

    // 创建UI元素
    let problemPanel = null;

    function createUI() {
        // 主按钮
        const btn = document.createElement('button');
        btn.id = 'ratingButton';
        btn.textContent = '获取题目推荐';

        // 题目面板
        problemPanel = document.createElement('div');
        problemPanel.className = 'problem-panel';

        document.body.appendChild(btn);
        document.body.appendChild(problemPanel);
        return btn;
    }

    // 渲染题目列表
    function renderProblems(data) {
        problemPanel.innerHTML = data.feedList.map(problem => `
            <div class="problem-item">
                <div style="flex: 1; margin-right: 12px;">
                    <div class="problem-title">${problem.title}</div>
                    <div class="problem-rating" style="color: ${getRatingColor(problem.rating)};">
                        难度：${problem.rating}
                    </div>
                </div>
                <button
                    class="jump-button"
                    onclick="window.open('${problem.url}', '_blank')"
                >
                    查看
                </button>
            </div>
        `).join('');

        problemPanel.style.display = 'block';
    }

    // 根据难度获取颜色
    function getRatingColor(rating) {
        if (rating >= 2200) return '#ff0000';
        if (rating >= 1900) return '#ff8c00';
        if (rating >= 1600) return '#ffd700';
        return '#228B22';
    }

    // 更新状态显示
    function updateStatus(message, color = '#333') {
        const statusDiv = document.querySelector('.rating-status');
        if (statusDiv) {
            statusDiv.textContent = message;
            statusDiv.style.color = color;
            // 3秒后淡出
            setTimeout(() => {
                statusDiv.style.opacity = '0';
                setTimeout(() => {
                    statusDiv.textContent = '';
                    statusDiv.style.opacity = '1';
                }, 1000);
            }, 3000);
        }
    }

    // 获取当前登录用户名
    function getLoggedInUsername() {
        const profileLink = document.querySelector('a[href^="/profile/"]');
        if (!profileLink) {
            updateStatus('未检测到登录用户', 'red');
            return null;
        }
        return profileLink.textContent.trim();
    }

    // 发送数据到后端服务
    function sendToBackend(username, rating) {
        GM_xmlhttpRequest({
            method: "POST",
            url: "http://36.111.157.95:8010/HttpService/Echo",
            headers: {
                "Content-Type": "application/json"
            },
            data: JSON.stringify({
                username: username,
                rating: rating
            }),
            onload: function(response) {
                try {
                    const data = JSON.parse(response.responseText);
                    if (response.status === 200) {
                        updateStatus('推荐题目已更新', 'green');
                        if (data.feedList) {
                            renderProblems(data);
                        }
                    } else {
                        updateStatus(`错误：${data.message}`, 'red');
                    }
                } catch (error) {
                    updateStatus('数据解析失败', 'red');
                }
            },
            onerror: function() {
                updateStatus('连接后端失败', 'red');
            }
        });
    }

    // 获取Rating主逻辑
    async function fetchAndDisplayRating() {
        updateStatus('正在获取...', '#2196F3');

        const username = getLoggedInUsername();
        if (!username) return;

        try {
            const rating = await new Promise((resolve, reject) => {
                GM_xmlhttpRequest({
                    method: "GET",
                    url: `https://codeforces.com/api/user.info?handles=${encodeURIComponent(username)}`,
                    onload: function(response) {
                        const data = JSON.parse(response.responseText);
                        if (data.status === 'OK') {
                            resolve(data.result[0].rating || '未评级');
                        } else {
                            reject(data.comment || '获取失败');
                        }
                    },
                    onerror: reject
                });
            });

            updateStatus(`当前Rating: ${rating}`, '#4CAF50');
            sendToBackend(username, rating);
        } catch (error) {
            console.error('获取Rating失败:', error);
            updateStatus(`错误: ${error.message || error}`, 'red');
        }
    }

    // 初始化
    function init() {
        const btn = createUI();
        btn.addEventListener('click', () => {
            problemPanel.style.display = 'none'; // 每次点击先隐藏面板
            fetchAndDisplayRating();
        });

        // 点击外部关闭面板
        document.addEventListener('click', (e) => {
            if (!problemPanel.contains(e.target) && e.target.id !== 'ratingButton') {
                problemPanel.style.display = 'none';
            }
        });
    }

    // 页面加载完成后初始化
    window.addEventListener('load', init, false);
})();