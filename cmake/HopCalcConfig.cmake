find_path(HopCalc_INCLUDE
    NAMES hopcalc.hpp
    PATHS /usr/local/include/hopcalc /usr/include/hopcalc)

find_library(HopCalc_LIB
    NAMES hopcalc
    PATHS /usr/local/lib /usr/lib)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(HopCalc
    REQUIRED_VARS HopCalc_LIB HopCalc_INCLUDE)

mark_as_advanced(HopCalc_INCLUDE HopCalc_LIB)
