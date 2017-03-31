#pragma once

#include <vector>
#include <functional>

#include "calg/expression.h"

namespace calg{

struct eval_context{
        using erased_transform_t = std::function<bool(expr::handle&)>;
        eval_context&  push(erased_transform_t t){
                transforms_.push_back(std::move(t));
                return *this;
        }
        auto begin(){ return transforms_.begin(); }
        auto end(){ return transforms_.end(); }
private:
        std::vector<erased_transform_t> transforms_;
};

inline
bool eval_once(eval_context& ctx, expr::handle& root){
        bool changed = false;
        for( auto& t : ctx ){
                bool result{t(root)};
                changed = changed || result;
        }
        return changed; 
}


inline
void eval(eval_context& ctx, expr::handle& root){
        for(;;){
                bool changed{eval_once(ctx, root)};
                if( ! changed )
                        break;
        }
}

}// calc
