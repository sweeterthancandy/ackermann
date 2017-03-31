#pragma once

namespace calg{
namespace transform{

struct constant_folding{
        bool operator()(expr::handle& root)const{
                return apply_bottom_up( [](expr::handle& expr)->bool{
                        using match_dsl::_c;
                        using match_dsl::match;
                        if( match( expr, _c + _c || _c - _c ) ){
                                auto ptr{ reinterpret_cast<operator_*&>(expr) };
                                expr::value_type arg0{ reinterpret_cast<constant*>(ptr->get_arg(0).get())->get_value()};
                                expr::value_type arg1{ reinterpret_cast<constant*>(ptr->get_arg(1).get())->get_value()};

                                if( ptr->get_name() == "+" ){
                                        expr = expr::handle{new constant(arg0 + arg1)};
                                } else if( ptr->get_name() == "-" ){
                                        expr = expr::handle{new constant(arg0 - arg1)};
                                }
                                return true;
                        }
                        return false;
                }, root);
        }
};

} // calg
} // transform
