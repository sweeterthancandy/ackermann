#pragma once

#include "calg/factory.h"
#include "calg/matcher.h"
#include "calg/algorithm.h"

namespace calg{
namespace transform{
/*
		         / y +1                 if x = 0
		A(x,y) = | A(x-1, 1)            if x > 0 and y = 0
                         \ A(x-1, A(x,y-1))     if x > 0 and y > 0
*/
struct ackermann_function{
private:
        factory fac;
        expr::handle _1;
public:
        ackermann_function(){
                _1 = fac.constant(1);
        }
        bool operator()(expr::handle& root){
                return apply_bottom_up( [this](expr::handle& root)->bool{
                        using match_dsl::_;
                        using match_dsl::_c;
                        using match_dsl::_call;
                        using match_dsl::match;
                        auto c{ reinterpret_cast<call*>( root.get()) };
                        if( match( root, _call("A", _c(0), _) ) ){
                                root = fac.operator_("+", c->get_arg(1), _1);
                                return true;
                        } else if( match( root, _call("A", _c, _c(0) ) ) ){
                                // A(x-1, 1)
                                root = fac.call("A", 
                                                fac.operator_("-", c->get_arg(0), _1),
                                                _1);
                                return true;
                        } else if( match( root, _call("A", _c, _c ) ) ){
                                // A(x,n) = A(x-1, A(x, n-1))
                                root = fac.call("A", 
                                                fac.operator_("-", c->get_arg(0), _1),
                                                fac.call("A", 
                                                         c->get_arg(0),
                                                         fac.operator_("-", c->get_arg(1), _1)));
                                return true;
                        }
                        return false;
                }, root);
        }
};

} // calg
} // transform
