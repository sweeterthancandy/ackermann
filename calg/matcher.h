#pragma once

namespace calg{
namespace match_dsl{

        struct any_matcher{
                bool match(expr::handle expr)const{
                       return true;
                }
                auto __to_matcher__()const{ return *this; }
        };

        struct kind_matcher{
                explicit kind_matcher(expr::kind k):k_{k}{}
                bool match(expr::handle expr)const{
                       return expr->get_kind() == k_;
                }
                auto __to_matcher__()const{ return *this; }
        private:
                expr::kind k_;
        };
        
        struct constant_val_matcher{
                explicit constant_val_matcher(expr::value_type val):val_(val){}
                bool match(expr::handle expr)const{
                       return expr->get_kind() == expr::kind_constant &&
                              reinterpret_cast<constant*>(expr.get())->get_value() == val_;
                }
                auto __to_matcher__()const{ return *this; }
        private:
                expr::value_type val_;
        };

        struct constant_matcher{
                bool match(expr::handle expr)const{
                       return expr->get_kind() == expr::kind_constant;
                }
                auto operator()(expr::value_type val)const{
                        return constant_val_matcher(val);
                }
                auto __to_matcher__()const{ return *this; }
        };
        
        struct symbol_name_matcher{
                explicit symbol_name_matcher(std::string const& name):name_(name){}
                bool match(expr::handle expr)const{
                       return expr->get_kind() == expr::kind_symbol &&
                              reinterpret_cast<symbol*>(expr.get())->get_name() == name_;
                }
                auto __to_matcher__()const{ return *this; }
        private:
                std::string name_;
        };
        
        struct symbol_matcher{
                bool match(expr::handle expr)const{
                       return expr->get_kind() == expr::kind_symbol;
                }
                auto operator()(std::string const& name)const{
                        return symbol_name_matcher{name};
                }
                auto __to_matcher__()const{ return *this; }
        };

        
        template<class... Args>
        struct call_matcher{

                using args_t = boost::fusion::vector<Args...>;

                call_matcher(){}
                call_matcher(std::string const& name, Args... args):
                        name_{name},
                        args_{args...}
                {}
                bool match(expr::handle expr)const{
                       if( expr->get_kind() != expr::kind_call )
                               return false;
                       if( name_.empty() )
                               return true;
                       // if we're matching the name we're matching the arguments
                       auto c{ reinterpret_cast<call*>(expr.get()) };
                       if( name_.size() && c->get_name() != name_ )
                               return false;
                       if( sizeof...(Args) != c->get_arity())
                               return false;
                       bool ret{true};
                       auto idx{0};
                       boost::fusion::for_each( args_, [&](auto const& m){
                                auto result{m.match(c->get_arg(idx++))}; 
                                ret = ret && result;
                       });
                       return ret;
                }


                // _call("A",_,_)
                template<class... Brgs>
                auto operator()(std::string const& name, Brgs&&... args)const{
                        return call_matcher<std::decay_t<Brgs>...>(name, std::forward<Brgs>(args)...);
                }
                auto __to_matcher__()const{ return *this; }
        private:
                std::string name_;
                args_t args_;
        };


        template<class L, class R>
        struct operator_matcher{
                explicit operator_matcher(std::string const& name, L const& l, R const& r):
                         l_(l), r_(r),name_(name)
                {}
                explicit operator_matcher(L const& l, R const& r):
                        l_(l), r_(r)
                {}
                bool match(expr::handle expr)const{
                        if( expr->get_kind() != expr::kind_operator )
                                return false;
                        auto op{ reinterpret_cast<operator_*>(expr.get())};
                        // can have empty name to match any operator
                        if( name_.size() && op->get_name() != name_)
                                return false;
                        if( op->get_arity() != 2 )
                                return false;
                        return l_.match( op->get_arg(0) ) && r_.match( op->get_arg(1) );
                }
                auto __to_matcher__()const{ return *this; }
        private:
                L l_;
                R r_;
                // where name_ = "", any operator_ matcher
                std::string name_;
        };
        
        template<class L, class R>
        struct or_matcher{
                explicit or_matcher(L const& l, R const& r):
                        l_(l), r_(r)
                {}
                bool match(expr::handle expr)const{
                        return l_.match(expr) || r_.match(expr);
                }
                auto __to_matcher__()const{ return *this; }
        private:
                L l_;
                R r_;
        };

        template<class L, class R>
        auto operator+(L const& l, R const& r)
                ->decltype( operator_matcher<decltype(l.__to_matcher__()), decltype(r.__to_matcher__())>( "+", l.__to_matcher__(), r.__to_matcher__() ) )
        {
                return operator_matcher<decltype(l.__to_matcher__()), decltype(r.__to_matcher__())>( "+", l.__to_matcher__(), r.__to_matcher__() ); 
        }
        template<class L, class R>
        auto operator-(L const& l, R const& r)
                ->decltype( operator_matcher<decltype(l.__to_matcher__()), decltype(r.__to_matcher__())>( "-", l.__to_matcher__(), r.__to_matcher__() ) )
        {
                return operator_matcher<decltype(l.__to_matcher__()), decltype(r.__to_matcher__())>( "-", l.__to_matcher__(), r.__to_matcher__() ); 
        }
        template<class L, class R>
        auto operator||(L const& l, R const& r)
                ->decltype( or_matcher<decltype(l.__to_matcher__()), decltype(r.__to_matcher__())>{l,r} )
        {
                return or_matcher<decltype(l.__to_matcher__()), decltype(r.__to_matcher__())>{l,r};
        }

        template<class M>
        bool match( expr::handle expr, M const& m){
                return m.match(expr);
        }

        template<class L, class R>
        auto op(L const& l, R const& r)
                ->decltype( operator_matcher<decltype(l.__to_matcher__()), decltype(r.__to_matcher__())>{ l.__to_matcher__(), r.__to_matcher__() })
        {
                return operator_matcher<decltype(l.__to_matcher__()), decltype(r.__to_matcher__())>{ l.__to_matcher__(), r.__to_matcher__() };
        }



        static auto _ = any_matcher{};
        static auto _c = constant_matcher{};
        static auto _s = symbol_matcher{};
        static auto _call = call_matcher<>{};

} // match_dsl
} // calg
