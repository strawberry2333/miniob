// benchmark 桩文件。
// 预期它会成为 oblsm 的独立压测入口，职责包括：
// - 组织顺序写、随机读、范围扫描等典型负载；
// - 统计吞吐、延迟以及后台 flush/compaction 对前台请求的影响；
// - 为不同参数组合提供可重复的基准对比。
// 当前仓库还没有真正实现 oblsm 的压测程序，这里保留一个最小入口，
// 方便后续参考 LevelDB 的 `db_bench` 补齐完整 benchmark 能力。
int main() { return 0; }
