/*=====================================================================================

    Filename:     incumbent_trace.h

    Description:  Bounded, timestamp-accurate incumbent trace compression
        Version:  2.0

=====================================================================================*/

#pragma once

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <vector>

struct Incumbent_Point
{
  double time;
  double objective;
};

class Incumbent_Trace
{
public:
  static constexpr size_t k_default_max_points = 3000;

  Incumbent_Trace()
      : m_enabled(false), m_started(false), m_time_limit(0.0),
        m_bucket_width(0.0), m_live_stream(nullptr), m_live_bucket_idx(0),
        m_has_live_bucket(false)
  {
  }

  void configure(bool p_enabled,
                 double p_time_limit,
                 size_t p_max_points = k_default_max_points)
  {
    m_enabled = p_enabled;
    m_started = false;
    m_time_limit = p_time_limit;
    m_buckets.clear();
    m_live_stream = nullptr;
    m_has_live_bucket = false;
    if (!m_enabled)
      return;

    const size_t bucket_count = std::max<size_t>(p_max_points / 2, 1);
    m_buckets.resize(bucket_count);
    m_bucket_width = m_time_limit / static_cast<double>(bucket_count);
  }

  void start(std::chrono::steady_clock::time_point p_start_time,
             FILE* p_live_stream = stdout)
  {
    if (!m_enabled)
      return;
    for (Bucket& bucket : m_buckets)
      bucket.used = false;
    m_start_time = p_start_time;
    m_live_stream = p_live_stream;
    m_has_live_bucket = false;
    m_started = true;
  }

  void record(double p_objective)
  {
    if (!m_enabled || !m_started)
      return;
    const double elapsed =
        std::chrono::duration_cast<std::chrono::duration<double>>(
            std::chrono::steady_clock::now() - m_start_time)
            .count();
    record_at(elapsed, p_objective);
  }

  void record_at(double p_time, double p_objective)
  {
    if (!m_enabled || !std::isfinite(p_time) ||
        !std::isfinite(p_objective) || p_time > m_time_limit)
      return;
    p_time = std::max(p_time, 0.0);
    const size_t bucket_count = m_buckets.size();
    assert(bucket_count > 0);
    size_t bucket_idx = 0;
    if (m_bucket_width > 0.0)
    {
      bucket_idx = static_cast<size_t>(p_time / m_bucket_width);
      bucket_idx = std::min(bucket_idx, bucket_count - 1);
    }
    const Incumbent_Point point{p_time, p_objective};
    if (m_live_stream != nullptr)
    {
      assert(!m_has_live_bucket || bucket_idx >= m_live_bucket_idx);
      if (!m_has_live_bucket || bucket_idx > m_live_bucket_idx)
      {
        output_live_bucket_last();
        output_point(m_live_stream, point);
        std::fflush(m_live_stream);
        m_live_bucket_idx = bucket_idx;
        m_has_live_bucket = true;
      }
    }
    Bucket& bucket = m_buckets[bucket_idx];
    if (!bucket.used)
    {
      bucket.first = point;
      bucket.used = true;
    }
    bucket.last = point;
  }

  void finish_live_output()
  {
    if (m_live_stream == nullptr)
      return;
    output_live_bucket_last();
    std::fflush(m_live_stream);
    m_live_stream = nullptr;
    m_has_live_bucket = false;
  }

  size_t point_count() const
  {
    size_t count = 0;
    for (const Bucket& bucket : m_buckets)
    {
      if (!bucket.used)
        continue;
      ++count;
      if (!same_point(bucket.first, bucket.last))
        ++count;
    }
    return count;
  }

  std::vector<Incumbent_Point> snapshot() const
  {
    std::vector<Incumbent_Point> points;
    points.reserve(point_count());
    for_each_point([&points](const Incumbent_Point& point)
                   { points.push_back(point); });
    return points;
  }

  void output(FILE* p_stream = stdout) const
  {
    for_each_point([p_stream](const Incumbent_Point& point)
                   { output_point(p_stream, point); });
  }

  size_t storage_bytes() const
  {
    return m_buckets.capacity() * sizeof(Bucket);
  }

private:
  struct Bucket
  {
    Incumbent_Point first;

    Incumbent_Point last;

    bool used;
  };

  bool m_enabled;

  bool m_started;

  double m_time_limit;

  double m_bucket_width;

  std::chrono::steady_clock::time_point m_start_time;

  std::vector<Bucket> m_buckets;

  FILE* m_live_stream;

  size_t m_live_bucket_idx;

  bool m_has_live_bucket;

  static void output_point(FILE* p_stream, const Incumbent_Point& p_point)
  {
    std::fprintf(p_stream,
                 "c [%14.9f] obj*: %-22.17g\n",
                 p_point.time,
                 p_point.objective);
  }

  void output_live_bucket_last()
  {
    if (!m_has_live_bucket)
      return;
    const Bucket& bucket = m_buckets[m_live_bucket_idx];
    assert(bucket.used);
    if (!same_point(bucket.first, bucket.last))
      output_point(m_live_stream, bucket.last);
  }

  static bool same_point(const Incumbent_Point& p_lhs,
                         const Incumbent_Point& p_rhs)
  {
    return p_lhs.time == p_rhs.time && p_lhs.objective == p_rhs.objective;
  }

  template <typename Callback>
  void for_each_point(Callback&& p_callback) const
  {
    size_t emitted = 0;
    for (const Bucket& bucket : m_buckets)
    {
      if (!bucket.used)
        continue;
      p_callback(bucket.first);
      ++emitted;
      if (!same_point(bucket.first, bucket.last))
      {
        p_callback(bucket.last);
        ++emitted;
      }
    }
    assert(emitted <= m_buckets.size() * 2);
  }
};
