#pragma once

#include "calg/expression.h"

namespace calg{

struct factory{
        using handle = expr::handle;

        handle constant(expr::value_type val){
                auto iter{constant_map_.find(val)};
                if( iter == constant_map_.end()){
                        expr::handle ret{ new ::calg::constant(val)};
                        constant_map_[val] = ret;
                        return ret;
                }
                return iter->second;
        }
        template<class... Args>
        handle operator_(Args&&... args)const{
                return expr::handle{ new ::calg::operator_(std::forward<Args>(args)...)};
        }
        template<class... Args>
        handle call(Args&&... args)const{
                return expr::handle{ new ::calg::call(std::forward<Args>(args)...)};
        }
private:
        std::map< int, expr::handle> constant_map_;
};

} // calg
