#pragma once

namespace calg{
namespace transform{

struct plus_folding{
        bool operator()(expr::handle& root)const{
                using match_dsl::_;
                using match_dsl::_c;
                using match_dsl::match;
                using match_dsl::op;
                bool changed{false};
                std::vector<expr::handle*> stack{&root};


                //std::cout << "root = " << *root << "\n";

                for(; stack.size(); ){
                        expr::handle& ptr{*stack.back()};
                        stack.pop_back();

                        // 1 + a + 2
                        //
                        // [+ + 1 a 2], []
                        // [+ 1 a, 2], []
                        // [+ 1 a], [2]
                        // [1, a], [2]
                        // [1], [2,a]
                        // [], [2,a,1]
                        //
                        // {0,-1,[]}       [2,a,1]
                        // f({0,-1,[]},1)  [2,a]
                        // f({1,0,[]},a)   [2]       
                        // f({1,0,[a]},2)  []
                        // {3,0,[a]}
                        //
                        // 3 + a
                        //
                        // a op b = inv( b op a )
                        // a op b op c 
                        // inv( b op a ) op c
                        // inv( c op inv(b op a ) )
                        // inv(c) op inv^2(b op a)
                        // inv(c) op ( b op a )
                        //
                        // 1 - 2
                        // - 1 2
                        // [-]  []
                        // [(+,1),(-,2)] []
                        // [(+,1)]   [(-,2)]
                        // []   [(+,1),(-,2])
                        //
                        // 1 + 2 - 3 - 2
                        // - - + 1 2 3 2
                        // [-] []
                        // [-,+] []
                        // [-,1,2] []
                        //
                        //
                        //

                        //PRINT_SEQ((stack.size()));

                        if( match( ptr, op(_,_) ) ){

                                auto op_ptr{ reinterpret_cast<operator_*>(ptr.get())};
                                
                                assert( detail::operator_traits.count( op_ptr->get_name() ) == 1 && "precondition failed");

                                auto traits = detail::operator_traits[ op_ptr->get_name() ];

                                //std::cout << "  found " << *ptr << std::endl;

                                // need to find the non-leaf chidlren
                                std::vector<expr::handle*> sub_stack{&ptr};
                                std::vector<expr::handle*> args;
                                for(;sub_stack.size();){
                                        auto& sub{*sub_stack.back()};
                                        sub_stack.pop_back();


                                        if( match(sub, op(_,_) ) ){
                                                auto op_sub{ reinterpret_cast<operator_*>(sub.get())};
                                                if( detail::operator_traits[op_sub->get_name()].precedence == traits.precedence )
                                                {
                                                        //std::cout << "    found sub " << *sub << "\n";
                                                        for( auto iter{op_sub->arg_begin()}, end{op_sub->arg_end()}; iter!=end;++iter){
                                                                sub_stack.emplace_back( &*iter );
                                                        }
                                                        continue;
                                                }
                                        }
                                        //std::cout << "    found arg " << *sub << "\n";
                                        // leaf
                                        args.emplace_back(&sub);
                                }
                                #if 0
                                std::cout << "args = ";
                                for( auto const& arg : args){
                                        std::cout << **arg << ",";
                                }
                                std::cout << "\n";
                                #endif
                                
                                #if 0
                                for( auto const& arg : args){
                                        stack.push_back( arg);
                                }
                                #endif

                                boost::sort( args, [](auto l, auto r){
                                        return (*l)->get_kind() < (*r)->get_kind();
                                });

                                factory fac;
                                expr::value_type c{0};
                                expr::handle ch;
                                std::vector<expr::handle> new_root;

                                for( auto iter{args.rbegin()}, end{args.rend()}; iter!=end;++iter){

                                        if( match( **iter, _c )){
                                           c += reinterpret_cast<constant*>((*iter)->get())->get_value();
                                           if( ! ch ){
                                                   ch = fac.constant(c);
                                                   new_root.push_back(ch);
                                           }
                                        } else{
                                                new_root.push_back( **iter );
                                        }

                                        if( new_root.size() == 2 ){
                                                auto op = fac.operator_("+", new_root[0], new_root[1]);
                                                new_root.clear();
                                                new_root.push_back(op);
                                        }
                                }
                                reinterpret_cast<constant*>(ch.get())->get_value() = c;

                                //std::cout << "new_root = " << *new_root << "\n";
                                ptr = new_root.back();

                        }
                        else if( ptr->get_kind() == expr::kind_call){
                                auto c{ reinterpret_cast<call*>(ptr.get()) };
                                for( auto iter{c->arg_begin()}, end{c->arg_end()}; iter!=end;++iter){
                                        stack.emplace_back( &*iter );
                                }
                        }
                }
                return changed;
        }
};


} // calg
} // transform
