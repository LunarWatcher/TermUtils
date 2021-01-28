#pragma once

#if __has_include(<optional>)
#include <optional>
template <typename T>
using StdOptional = std::optional<T>;
#else
#include <experimental/optional>
using StdOptional = std::experimental::optional;
#endif

#if __has_include(<filesystem>)
#include <filesystem>
namespace fs = std::filesystem;
#else
#include "experimental/filesystem"
namespace fs = std::experimental::filesystem;
#endif

