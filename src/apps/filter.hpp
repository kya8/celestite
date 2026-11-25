#ifndef CLST_APPS_FILTER_HPP
#define CLST_APPS_FILTER_HPP

#include <optional>
#include <algorithm>
#include <vector>
#include <cmath>

template<typename T>
auto sort_median(std::vector<T>& data) -> std::optional<T>
{
    if (data.empty())
        return std::nullopt;
    std::sort(data.begin(), data.end());
    const auto mid = data.size() / 2;
    if (data.size() % 2 == 0) {
        return (data[mid - 1] + data[mid]) / 2;
    }
    return data[mid];
}

template<typename T>
auto median(std::vector<T> data) -> std::optional<T> // Move in or copy. This function operates on its own copy of the data.
{
    return sort_median(data);
}

template<typename It>
auto median(It first, It last)
{
    return median(std::vector(first, last));
}

template<typename It>
auto get_outliers(It first, It last, double m = 5.0) -> std::vector<It>
{
    std::vector<It> result;
    std::vector data(first, last);
    const auto med = sort_median(data);
    if (!med)
        return result;

    for (auto& x : data) {
        x = std::abs(x - *med);
    }
    const auto mad = median(std::move(data));

    for (auto it = first; it != last; ++it) {
        if (std::abs(*it - *med) > m * *mad) {
            result.push_back(it);
        }
    }
    return result;
}

#endif /* CLST_APPS_FILTER_HPP */
