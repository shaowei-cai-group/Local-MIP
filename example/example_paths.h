#pragma once

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>

namespace example_paths
{

inline std::string resolve_existing_model_path_or_exit(
    const std::filesystem::path& model_path)
{
  if (std::filesystem::exists(model_path))
    return model_path.lexically_normal().string();

  std::fprintf(stderr,
               "c Example model file not found: %s\n",
               model_path.string().c_str());
  std::exit(EXIT_FAILURE);
}

inline std::string resolve_bundled_model_path_or_exit(
    const std::filesystem::path& relative_model_path)
{
  const std::filesystem::path local_path = relative_model_path;
  const std::filesystem::path parent_path =
      std::filesystem::path("..") / relative_model_path;

  if (std::filesystem::exists(local_path))
    return local_path.lexically_normal().string();
  if (std::filesystem::exists(parent_path))
    return parent_path.lexically_normal().string();

  std::fprintf(stderr,
               "c Example model file not found. Tried '%s' and '%s'.\n",
               local_path.string().c_str(),
               parent_path.string().c_str());
  std::exit(EXIT_FAILURE);
}

inline std::string resolve_demo_model_path_or_exit(
    int argc,
    char** argv,
    const std::filesystem::path& default_relative_model_path)
{
  if (argc > 1)
    return resolve_existing_model_path_or_exit(argv[1]);

  return resolve_bundled_model_path_or_exit(default_relative_model_path);
}

} // namespace example_paths
