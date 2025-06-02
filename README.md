RTNlite 轻量级实时通信引擎操作指南



RTNlite 是一款轻量级实时通信引擎，以下是在 MAC & Linux 系统上使用它的详细操作步骤：


一、安装 nodejs



在 MAC & Linux 系统上安装 nodejs，你可以根据系统类型，通过官方网站（[https://nodejs.or](https://nodejs.org/)[g/](https://nodejs.org/)）下载对应版本的安装包进行安装，或者使用系统的包管理工具（如在 Linux 上使用 apt、yum 等，在 MAC 上使用 Homebrew）进行安装。具体安装方法可参考 nodejs 官方文档。


二、启动服务器





1.  进入`example/web`目录。


2.  执行命令`node server.cjs`启动服务器。


三、获取 RoomId





1.  打开浏览器（建议使用 chrome 浏览器），访问`https://localhost:9081/`页面。


2.  在该页面获取`RoomId`，后续操作将用到此 ID。


四、编译生成可执行文件





1.  进入`example`目录。


2.  执行`make`命令，编译完成后将得到`hello_rtnlite`可执行文件。


## 五、运行`hello_rtnlite`

执行命令`./hello_rtnlite -u UserId -r RoomId`，其中`UserId`为你的用户标识，`RoomId`为第三步获取到的房间标识。


参考资料





1.  **详细演示视频**：RTNLite.mp4


2.  **技术文档**：[https://mp.wei](https://mp.weixin.qq.com/s/0TgTdN1VFrOUmm9CHJ1Luw)[xin](https://mp.weixin.qq.com/s/0TgTdN1VFrOUmm9CHJ1Luw)[.qq.com](https://mp.weixin.qq.com/s/0TgTdN1VFrOUmm9CHJ1Luw)[/s/0T](https://mp.weixin.qq.com/s/0TgTdN1VFrOUmm9CHJ1Luw)[gTdN1](https://mp.weixin.qq.com/s/0TgTdN1VFrOUmm9CHJ1Luw)[VFrOU](https://mp.weixin.qq.com/s/0TgTdN1VFrOUmm9CHJ1Luw)[mm9CH](https://mp.weixin.qq.com/s/0TgTdN1VFrOUmm9CHJ1Luw)[J1Luw](https://mp.weixin.qq.com/s/0TgTdN1VFrOUmm9CHJ1Luw)

3.  **RTNlite vs 声网 RTSA 内存 / 包大小对比**：[https://tknugg6xx5.feishu](https://tknugg6xx5.feishu.cn/docx/G9bXduFOiodO3XxUSRQcXgT2nBb?from=from_copylink)[.cn/docx/G9bXduFOiodO3XxUSRQcXgT2nBb?from=from\_copylink](https://tknugg6xx5.feishu.cn/docx/G9bXduFOiodO3XxUSRQcXgT2nBb?from=from_copylink)
