import axios from "axios"   

const globalCookie = '__client_id=259c7e72e2e5e5a39e563a119577318ca0d9d33f; _uid=1093351; C3VK=8bcc12';

const http = axios.create({
    headers: {
        'Cookie': globalCookie,
        },
    withCredentials: true,
})
function decode(str) {
return str.replace(/\\x(\w{2})/g, function(_, $1) {
return String.fromCharCode(parseInt($1, 16));
});
}

async function getSolution() {
    const res = decode((await http.get("https://www.luogu.com.cn/problem/solution/CF2055C")).data)
    console.log(res);

    const newCookie = res.match(/(?<=cookie=").*?;/)
    console.log(newCookie[0]);
    const solutionRes = await http.get("https://www.luogu.com.cn/problem/solution/CF2055C", {
        Headers: {'Cookie': globalCookie + newCookie[0]}
    })
    console.log(globalCookie + newCookie[0]);
    

}
getSolution()
