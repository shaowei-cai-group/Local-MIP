/*=====================================================================================

    Filename:     parallel_multiseed.cpp

    Description:  External multi-seed parallelism over one Prepared_Model
        Version:  2.0

=====================================================================================*/

#include "example_paths.h"
#include "local_mip/Local_MIP.h"
#include "model_data/Prepared_Model.h"
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <limits>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace
{

constexpr const char kDefaultModelFile[] = "test-set/allcolor10.mps";

} // namespace

int main(int argc, char** argv)
{
  const std::string model_file = example_paths::resolve_demo_model_path_or_exit(
      argc, argv, kDefaultModelFile);

  Model_Prepare_Options prepare_options;
  auto prepared =
      Prepared_Model::from_file(model_file, prepare_options);
  const std::vector<std::uint32_t> seeds = {1, 2, 3, 4};

  std::vector<std::unique_ptr<Local_MIP>> solvers;
  solvers.reserve(seeds.size());
  for (std::uint32_t seed : seeds)
  {
    auto solver = std::make_unique<Local_MIP>(prepared);
    solver->set_random_seed(seed);
    solver->set_time_limit(10.0);
    solver->set_log_obj(false);
    std::printf("seed %u model address: %p\n",
                seed,
                static_cast<const void*>(solver->get_model_manager()));
    solvers.push_back(std::move(solver));
  }

  std::vector<std::exception_ptr> errors(seeds.size());
  std::vector<std::thread> workers;
  workers.reserve(seeds.size());
  for (std::size_t idx = 0; idx < seeds.size(); ++idx)
  {
    Local_MIP* solver = solvers[idx].get();
    workers.emplace_back(
        [solver, &errors, idx]()
        {
          try
          {
            solver->run();
          }
          catch (...)
          {
            errors[idx] = std::current_exception();
          }
        });
  }
  for (auto& worker : workers)
    worker.join();

  const bool minimize = prepared->model_manager().is_min() > 0;
  std::size_t best_idx = seeds.size();
  bool worker_failed = false;
  double best_obj = minimize ? std::numeric_limits<double>::infinity()
                             : -std::numeric_limits<double>::infinity();
  for (std::size_t idx = 0; idx < seeds.size(); ++idx)
  {
    if (errors[idx] != nullptr)
    {
      worker_failed = true;
      try
      {
        std::rethrow_exception(errors[idx]);
      }
      catch (const std::exception& error)
      {
        std::fprintf(stderr,
                     "seed %u failed: %s\n",
                     seeds[idx],
                     error.what());
      }
      catch (...)
      {
        std::fprintf(stderr, "seed %u failed with an unknown error\n",
                     seeds[idx]);
      }
      continue;
    }
    if (!solvers[idx]->is_feasible())
    {
      std::printf("seed %u: no feasible solution\n", seeds[idx]);
      continue;
    }

    const double objective = solvers[idx]->get_obj_value();
    std::printf("seed %u: objective %.15g\n", seeds[idx], objective);
    if (!std::isfinite(objective))
      continue;
    if (best_idx == seeds.size() ||
        (minimize ? objective < best_obj : objective > best_obj))
    {
      best_idx = idx;
      best_obj = objective;
    }
  }

  if (best_idx == seeds.size())
  {
    std::printf("No seed produced a finite feasible objective.\n");
    return 1;
  }
  std::printf("Best seed: %u, objective: %.15g\n",
              seeds[best_idx],
              best_obj);
  return worker_failed ? 1 : 0;
}
