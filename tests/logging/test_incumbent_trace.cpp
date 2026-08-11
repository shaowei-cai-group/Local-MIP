/*=====================================================================================

    Filename:     test_incumbent_trace.cpp

    Description:  Bounded incumbent trace tests
        Version:  2.0

=====================================================================================*/

#include "local_search/incumbent_trace.h"
#include <cmath>
#include <cstdio>
#include <vector>

namespace
{

bool check(bool p_condition, const char* p_message)
{
  if (!p_condition)
    std::fprintf(stderr, "ERROR: %s\n", p_message);
  return p_condition;
}

bool test_bounded_trace()
{
  Incumbent_Trace trace;
  trace.configure(true, 1.0);
  for (size_t idx = 0; idx < 1000000; ++idx)
  {
    trace.record_at(static_cast<double>(idx) / 1000000.0,
                    1000000.0 - static_cast<double>(idx));
  }
  trace.record_at(1.1, -1.0);

  const std::vector<Incumbent_Point> points = trace.snapshot();
  bool ok = true;
  ok &= check(points.size() <= Incumbent_Trace::k_default_max_points,
              "trace must respect its hard point cap");
  ok &= check(points.front().time == 0.0 &&
                  points.front().objective == 1000000.0,
              "trace must preserve the first incumbent");
  ok &= check(points.back().time == 0.999999 &&
                  points.back().objective == 1.0,
              "trace must preserve the final in-limit incumbent");
  for (size_t idx = 1; idx < points.size(); ++idx)
  {
    ok &= check(points[idx - 1].time <= points[idx].time,
                "trace timestamps must be monotone");
  }
  ok &= check(trace.storage_bytes() <= 64 * 1024,
              "default trace storage must stay below 64 KiB");
  return ok;
}

bool test_bucket_and_output_semantics()
{
  Incumbent_Trace trace;
  trace.configure(true, 1.0, 4);
  trace.record_at(0.1, 10.0);
  trace.record_at(0.2, 9.0);
  trace.record_at(0.3, 8.0);
  trace.record_at(0.6, 7.0);

  const std::vector<Incumbent_Point> points = trace.snapshot();
  bool ok = true;
  ok &= check(points.size() == 3 && points[0].objective == 10.0 &&
                  points[1].objective == 8.0 &&
                  points[2].objective == 7.0,
              "each bucket should keep its first and last incumbent");

  FILE* output = std::tmpfile();
  ok &= check(output != nullptr, "temporary output file should open");
  if (output == nullptr)
    return false;
  trace.output(output);
  std::rewind(output);
  char line[256];
  size_t line_count = 0;
  while (std::fgets(line, sizeof(line), output) != nullptr)
  {
    double time = 0.0;
    double objective = 0.0;
    ok &= check(std::sscanf(line,
                            "c [%lf] obj*: %lf",
                            &time,
                            &objective) == 2 &&
                    std::isfinite(time) && std::isfinite(objective),
                "trace output must remain parser-compatible");
    ++line_count;
  }
  std::fclose(output);
  ok &= check(line_count == points.size(),
              "trace output should contain one line per stored point");
  return ok;
}

bool test_disabled_trace()
{
  Incumbent_Trace trace;
  trace.configure(false, 1.0);
  trace.record_at(0.0, 1.0);
  return check(trace.point_count() == 0 && trace.storage_bytes() == 0,
               "disabled trace must use no dynamic storage");
}

bool test_incremental_output()
{
  FILE* output = std::tmpfile();
  if (!check(output != nullptr, "temporary live output file should open"))
    return false;

  Incumbent_Trace trace;
  trace.configure(true, 1.0, 4);
  trace.start(std::chrono::steady_clock::now(), output);
  trace.record_at(0.1, 10.0);

  bool ok = true;
  ok &= check(std::ftell(output) > 0,
              "the first incumbent should be flushed immediately");

  trace.record_at(0.2, 9.0);
  trace.record_at(0.3, 8.0);
  trace.record_at(0.6, 7.0);
  trace.record_at(0.7, 6.0);
  trace.finish_live_output();
  trace.finish_live_output();

  std::rewind(output);
  const double expected[] = {10.0, 8.0, 7.0, 6.0};
  char line[256];
  size_t line_count = 0;
  while (std::fgets(line, sizeof(line), output) != nullptr)
  {
    double time = 0.0;
    double objective = 0.0;
    const bool parsed = std::sscanf(line,
                                    "c [%lf] obj*: %lf",
                                    &time,
                                    &objective) == 2;
    ok &= check(parsed, "incremental trace line should remain parseable");
    ok &= check(line_count < 4,
                "incremental trace should not emit duplicate points");
    if (parsed && line_count < 4)
      ok &= check(objective == expected[line_count],
                  "incremental trace should preserve snapshot order");
    ++line_count;
  }
  std::fclose(output);
  ok &= check(line_count == 4,
              "incremental trace should emit each retained point once");
  return ok;
}

} // namespace

int main()
{
  const bool ok = test_bounded_trace() &&
                  test_bucket_and_output_semantics() &&
                  test_disabled_trace() &&
                  test_incremental_output();
  if (ok)
    std::printf("All incumbent trace tests passed.\n");
  return ok ? 0 : 1;
}
