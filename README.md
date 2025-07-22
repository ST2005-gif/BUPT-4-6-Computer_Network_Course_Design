# 北邮计科大二下课程——计算机网络课程设计
## 2025年春
### 课程设计简介

该项目为设计一个DNS中继服务器

项目为三人项目

代码思路参考了[这位学长](https://github.com/yige2021/LyDNS_new)

[项目说明](https://github.com/ST2005-gif/Course-Learning-Tips-from-bupt-scs/tree/main/%E5%A4%A7%E4%BA%8C%E4%B8%8B)

认为有用的话就⭐一下吧！😃

### 项目设定
使用语言：C

编译平台：Visual Studio

### 运行方式
建议使用Visual Studio创建项目，将所有.h文件添加至头文件，将.c文件添加至源文件，将dnsrelay.txt添加到资源文件，最后编译运行

在项目开始运行后，使用powershell或cmd命令行输入内容进行测试，如nslookup www.baidu.com 127.0.0.1

### 未解决的问题及bug

1.仍未解决CPU“忙等待”的问题

2.查询结果输出不够规范，一个网址若同时存在ipv4地址与ipv6地址，则会全部输出这两个地址

3.多线程功能无法测试

### 项目架构

```
BUPT-4-2-Cpp_Course_Design/
├── 📂 项目/        
│   ├── 计网课设报告.docx                          
│   └── 📂 src/                           
├── 📂 要求/                          # 要求文档
│   └── 计算机网络课程设计-DNS.pptx
└── 📄 README.md
```
