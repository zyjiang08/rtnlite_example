# RTNLite_Release
MAC
1. 安装nodejs
#brew install node

2. 启动服务器
进入example/web，执行：
#node server.cjs 

3. 打开https://localhost:9081/页面，获取RoomId. 建议使用chrome浏览器

4. 进入example目录，执行make，得到hello_rtnlite

5. 执行hello_rtnlite
#./hello_rtnlite -u UserId -r RoomId