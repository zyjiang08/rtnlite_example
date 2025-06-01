# WebRTC 信令服务器

这是WebRTC信令服务器的独立部署包。

## 文件说明
- server.cjs: 独立服务器可执行文件
- index.html: Web客户端界面
- css/: 样式文件
- nginx/cert/: SSL证书
- start.sh: 便捷启动脚本

## 使用方法
启动服务器:

```bash
./start.sh
# 或者
node server.cjs
```

服务器将在 https://localhost:9081 上可用
