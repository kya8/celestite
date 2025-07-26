#ifndef CLST_CONSTANTS_HPP
#define CLST_CONSTANTS_HPP

namespace clst::math {

template<typename T = double>
constexpr T sqrt_2_v = static_cast<T>(1.414213562373095048801688724209698078L);
template<typename T = double>
constexpr T sqrt_3_v = static_cast<T>(1.732050807568877293527446341505872366L);

template<typename T = double>
constexpr T e_v = static_cast<T>(2.718281828459045235360287471352662497L);
template<typename T = double>
constexpr T sqrt_e_v = static_cast<T>(1.648721270700128146848650787814163571L);
template<typename T = double>
constexpr T ln_2_v = static_cast<T>(0.693147180559945309417232121458176568L);

template<typename T = double>
constexpr T pi_v = static_cast<T>(3.141592653589793238462643383279502884L);
template<typename T = double>
constexpr T sqrt_pi_v = static_cast<T>(1.772453850905516027298167483341145182L);

template<typename T = double>
constexpr T euler_v = static_cast<T>(0.577215664901532860606512090082402431L);

template<typename T = double>
constexpr T rad2deg_v = static_cast<T>(57.29577951308232087679815481410517033L);
template<typename T = double>
constexpr T deg2rad_v = T{1.0} / rad2deg_v<T>;

constexpr double sqrt_2 = sqrt_2_v<double>;
constexpr double sqrt_3 = sqrt_3_v<double>;
constexpr double e = e_v<double>;
constexpr double sqrt_e = sqrt_e_v<double>;
constexpr double ln_2 = ln_2_v<double>;
constexpr double pi = pi_v<double>;
constexpr double sqrt_pi = sqrt_pi_v<double>;
constexpr double euler = euler_v<double>;
constexpr double rad2deg = rad2deg_v<double>;
constexpr double deg2rad = deg2rad_v<double>;

}

#endif // CLST_CONSTANTS_HPP
