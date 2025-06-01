#!/bin/bash

# 生成私钥，保存为 .key 格式
openssl genpkey -algorithm RSA -out private.key -pkeyopt rsa_keygen_bits:2048

# 生成 CSR
openssl req -new -key private.key -out certificate.csr <<EOF
CN
State
City
Organization
OrganizationalUnit
localhost
2935616@qq.com


EOF

# 自签名证书，保存为 .pem 格式
openssl x509 -req -in certificate.csr -signkey private.key -out certificate.pem -days 365

echo "本地证书生成完成，私钥文件: private.key，证书文件: certificate.pem"
