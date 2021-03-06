#include <boost/dll.hpp>

#include "action/UD0001/UD0001.hpp"
#include "action/UD0003/UD0003.hpp"
#include "action/UD0004/UD0004.hpp"

// UD : user-defined
// AD : auto-defined

#define API extern "C" BOOST_SYMBOL_EXPORT

BOOST_DLL_ALIAS(solution::action::UD0001::run, UD0001); // MOEX, QUIK, TA, Stock Exchange, ML-models (python)
BOOST_DLL_ALIAS(solution::action::UD0003::run, UD0003); // MOEX, QUIK, TA, Stock Exchange, View, Window 
BOOST_DLL_ALIAS(solution::action::UD0004::run, UD0004); // MOEX, QUIK, Quotes analysis, Stock Exchange, View, Window 