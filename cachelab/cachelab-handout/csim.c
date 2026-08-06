#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "cachelab.h"

int s = 5;          // set index bits
int E = 1;          // associativity (lines per set)
int b = 5;          // block offset bits
int S = 0;          // number of sets (2^s)
int B = 0;          // block size in bytes (2^b)
int verbosity = 0;  // verbose flag
int hit_count = 0;
int miss_count = 0;
int eviction_count = 0;
char* trace_file = NULL;

typedef struct cache_line {
    unsigned char valid;     // 0 or 1
    unsigned long long tag;  // 地址标记
    unsigned long long lru;
} cache_line_t;

cache_line_t** cache = NULL;            // cache[set][line]
unsigned long long lru_counter = 0;     // 全局递增计数器
unsigned long long set_index_mask = 0;  // 用于快速提取组索引（但可直接计算）

void initCache() {
    S = 1 << s;
    B = 1 << b;
    // 分配 cache 指针数组 (S个组)
    cache = (cache_line_t**)malloc(S * sizeof(cache_line_t*));

    for (int i = 0; i < S; i++) {
        // 为每组分配 E 个行
        cache[i] = (cache_line_t*)malloc(E * sizeof(cache_line_t));
        for (int j = 0; j < E; j++) {
            cache[i][j].valid = 0;
            cache[i][j].tag = 0;
            cache[i][j].lru = 0;
        }
    }

    // 计算 set_index_mask (用于快速提取组索引，但后续直接计算)
    set_index_mask = (1ull << s) - 1;
}

void freeCache() {
    for (int i = 0; i < S; i++) {
        free(cache[i]);
    }
    free(cache);
}

void accessData(unsigned long long address) {
    // 计算组索引和标记
    int set_index = (address >> b) & set_index_mask;
    unsigned long long tag = address >> (s + b);

    cache_line_t* line_set = cache[set_index];
    int victim = -1;
    unsigned long long min_lru;

    // 先检查是否命中
    for (int i = 0; i < E; i++) {
        if (line_set[i].valid && line_set[i].tag == tag) {
            // 命中
            hit_count++;
            if (verbosity) {
                printf("hit ");
            }
            // 更新 LRU (设置为当前最新)
            line_set[i].lru = ++lru_counter;
            return;
        }
    }

    // 未命中
    miss_count++;
    if (verbosity) {
        printf("miss ");
    }

    // 查找空行或替换
    int empty = -1;
    for (int i = 0; i < E; i++) {
        if (!line_set[i].valid) {
            empty = i;
            break;
        }
    }

    if (empty != -1) {
        victim = empty;
    } else {
        // 没有空行，需要驱逐（LRU）
        eviction_count++;
        if (verbosity) {
            printf("eviction ");
        }
        victim = 0;
        min_lru = line_set[0].lru;
        for (int i = 1; i < E; i++) {
            if (line_set[i].lru < min_lru) {
                min_lru = line_set[i].lru;
                victim = i;
            }
        }
    }

    // 加载新行
    line_set[victim].valid = 1;
    line_set[victim].tag = tag;
    line_set[victim].lru = ++lru_counter;
}

void replayTrace(char* tracefile) {
    FILE* fp = fopen(tracefile, "r");
    if (!fp) {
        fprintf(stderr, "%s: %s\n", tracefile, strerror(errno));
        exit(1);
    }

    char line[1000];
    while (fgets(line, sizeof(line), fp)) {
        char op;
        unsigned long long addr;
        int size;
        if (line[0] == 'I') {
            continue;
        }  // 忽略指令访问

        // 使用sscanf跳过前导空格
        if (sscanf(line, " %c %llx,%d", &op, &addr, &size) != 3) {
            continue;
        }

        if (verbosity) {
            printf("%c %llx,%d ", op, addr, size);
        }

        switch (op) {
            case 'L':
            case 'S':
                accessData(addr);
                break;
            case 'M':
                accessData(addr);  // load
                accessData(addr);  // store
                break;
            default:
                break;
        }

        if (verbosity) {
            putchar('\n');
        }
    }
    fclose(fp);
}

void printUsage(char** argv) {
    printf("Usage: %s [-hv] -s <num> -E <num> -b <num> -t <file>\n", argv[0]);
    puts("Options:");
    puts("  -h         Print this help message.");
    puts("  -v         Optional verbose flag.");
    puts("  -s <num>   Number of set index bits.");
    puts("  -E <num>   Number of lines per set.");
    puts("  -b <num>   Number of block offset bits.");
    puts("  -t <file>  Trace file.");
    puts("\nExamples:");
    printf("  linux>  %s -s 4 -E 1 -b 4 -t traces/yi.trace\n", argv[0]);
    printf("  linux>  %s -v -s 8 -E 2 -b 4 -t traces/yi.trace\n", argv[0]);
    exit(0);
}

int main(int argc, char* argv[]) {
    int opt;
    opterr = 0;  // 关闭自动错误信息

    while ((opt = getopt(argc, argv, "hvs:E:b:t:")) != -1) {
        switch (opt) {
            case 'h':
                printUsage(argv);
                break;
            case 'v':
                verbosity = 1;
                break;
            case 's':
                s = atoi(optarg);
                break;
            case 'E':
                E = atoi(optarg);
                break;
            case 'b':
                b = atoi(optarg);
                break;
            case 't':
                trace_file = optarg;
                break;
            default:
                printUsage(argv);
                return 1;
        }
    }

    // 检查必需参数
    if (s == 0 || E == 0 || b == 0 || trace_file == NULL) {
        printf("%s: Missing required command line argument\n", argv[0]);
        printUsage(argv);
        return 1;
    }

    initCache();
    replayTrace(trace_file);
    freeCache();
    printSummary(hit_count, miss_count, eviction_count);

    return 0;
}
