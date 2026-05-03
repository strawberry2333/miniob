#pragma once
// 这是仅供 IDE（clangd）静态分析使用的 Google Benchmark 最小 stub。
// 不参与实际编译，真正编译时需要安装 Google Benchmark 库。
#include <cstdint>
#include <string>

namespace benchmark {

// 链式调用返回类型，支持 ->Arg() ->Range() ->RangeMultiplier()
struct BenchmarkRegistrar
{
  BenchmarkRegistrar *Arg(int64_t) { return this; }
  BenchmarkRegistrar *Range(int64_t, int64_t) { return this; }
  BenchmarkRegistrar *RangeMultiplier(int) { return this; }
};

// 测试运行时状态，每个 benchmark 函数接收此对象
class State
{
public:
  // 返回通过 Arg()/Range() 注册的参数值，index 对应参数位置
  int64_t range(int index = 0) const { return 0; }

  // 支持 for (auto _ : state) 语法，控制 benchmark 的迭代
  struct Iterator
  {
    bool        operator!=(const Iterator &) const { return false; }
    void        operator++() {}
    struct Noop {};
    Noop operator*() { return {}; }
  };
  Iterator begin() { return {}; }
  Iterator end() { return {}; }
};

// Fixture 基类，SetUp/TearDown 在每轮测试前后调用
class Fixture
{
public:
  virtual void SetUp(const State &) {}
  virtual void TearDown(const State &) {}
  virtual ~Fixture() = default;
};

// 防止编译器将被测变量优化掉，确保测试结果有效
template <typename T>
void DoNotOptimize(T &val)
{
  asm volatile("" : "+r"(val));
}

template <typename T>
void DoNotOptimize(T &&val)
{}

BenchmarkRegistrar *RegisterBenchmark(const char *, void (*)(State &));

}  // namespace benchmark

// 定义一个 Fixture 成员方法作为测试用例体
#define BENCHMARK_DEFINE_F(fixture, name) \
  void fixture##_##name##_impl(benchmark::State &state); \
  void fixture##_##name##_impl

// 注册 Fixture 测试用例，返回链式调用对象
#define BENCHMARK_REGISTER_F(fixture, name) \
  (new benchmark::BenchmarkRegistrar())

// 注册普通函数作为测试用例
#define BENCHMARK(func) \
  (new benchmark::BenchmarkRegistrar())

// benchmark 程序的 main 函数入口
#define BENCHMARK_MAIN() \
  int main(int argc, char **argv) { return 0; }
