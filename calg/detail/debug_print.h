#pragma once

#define PRINT_SEQ_detail(r, d, i, e) do{ std::cout << ( i ? ", " : "" ) << BOOST_PP_STRINGIZE(e) << " = " << (e); }while(0);
#define PRINT_SEQ(SEQ) do{ BOOST_PP_SEQ_FOR_EACH_I( PRINT_SEQ_detail, ~, SEQ) std::cout << "\n"; }while(0)
#define PRINT_TEST( EXPR ) do{ auto ret{EXPR}; std::cout << std::setw(50) << std::left << #EXPR << std::setw(0) << ( ret ? " OK" : " FAILED" ) << "\n"; }while(0);

