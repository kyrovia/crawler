# crawler

用 C++17 写的目录爬虫，练习两种并发下载：阻塞队列（`crawler`）和线程池（`crawler_concurrent`）。

默认从清华 GNU `ed` 镜像抓文件，下载到当前目录下的 `gnu/ed/`。

## 构建

依赖：C++17 编译器、CMake 3.16+、libcurl。测速测试还需要 GoogleTest。

```bash
cmake -S . -B build
cmake --build build
```

## 运行

```bash
./build/crawler
./build/crawler_concurrent
```

可选参数覆盖起始 URL：

```bash
./build/crawler 'https://mirrors.tuna.tsinghua.edu.cn/gnu/ed/'
```
