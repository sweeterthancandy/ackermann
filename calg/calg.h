#pragma once



#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <numeric>
#include <memory>
#include <type_traits>
#include <iomanip>


#include <boost/range/algorithm.hpp>
#include <boost/preprocessor.hpp>
#include <boost/function.hpp>
#include <boost/functional/hash.hpp>
#include <boost/fusion/include/vector.hpp>
#include <boost/fusion/include/algorithm.hpp>


#include "calg/expression.h"
#include "calg/calg.h"
#include "calg/eval.h"
#include "calg/algorithm.h"
#include "calg/frontend.h"

#include "calg/detail/throw.h"
#include "calg/detail/debug_print.h"
#include "calg/detail/traits.h"

#include "calg/transform/ackermann_function.h"
#include "calg/transform/plus_folding.h"
#include "calg/transform/constant_folding.h"
#include "calg/transform/symbol_substitute.h"
