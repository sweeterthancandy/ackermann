#pragma once

namespace calg{

/*                  +                    +    
                  /   \                /    \
                +       1   ->      alpha    2
              /    \
           alpha    2
 
 */

template<class F>
bool apply_bottom_up(F f, expr::handle& root){
        bool result{false};
        enum class opcode{
                yeild_or_apply,
                apply
        };

        std::vector<std::pair<opcode, expr::handle*>> stack;
        stack.emplace_back(opcode::yeild_or_apply, &root);
        for(;stack.size();){
                auto p{stack.back()};
                stack.pop_back();

                switch(p.first){
                case opcode::yeild_or_apply:
                        if( (*p.second)->is_non_terminal()){
                                stack.emplace_back( opcode::apply, p.second );
                                auto c{ reinterpret_cast<call*>(p.second->get()) };
                                for( auto iter{c->arg_begin()}, end{c->arg_end()}; iter!=end;++iter){
                                        stack.emplace_back( opcode::yeild_or_apply, &*iter );
                                }
                                break;
                        }
                        // fallthought
                case opcode::apply:{
                        bool ret{f( *p.second )};
                        result = result || ret;
                }
                        break;
                }
        }
        return result;
}

} // calg
