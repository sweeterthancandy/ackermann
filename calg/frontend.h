#pragma once

#include "calg/expression.h"

namespace calg{
namespace frontend{

        struct frontend_wrapper{
                frontend_wrapper(){}
                explicit frontend_wrapper(expr::handle ptr):ptr_(ptr){}
                operator expr::handle(){ return ptr_; }
                auto to_expr(){ return ptr_; }
                auto to_expr()const{ return ptr_; }
                friend std::ostream& operator<<(std::ostream& ostr, frontend_wrapper const& self){
                        return ostr << *self.ptr_;
                }
        private:
                expr::handle ptr_;
        };

        inline
        auto const_(expr::value_type val){
                return frontend_wrapper{expr::handle{new constant{val}}};
        }
        inline
        auto sym(std::string const& name){
                return frontend_wrapper{expr::handle{new symbol{name}}};
        }

        namespace detail{
                template<class T>
                auto to_expr(T const& t)->decltype(expr::handle{new constant{t}})
                {
                        return expr::handle{new constant{t}};
                }
                template<class T>
                auto to_expr(T const& t)->decltype(expr::handle{new symbol{t}})
                {
                        return expr::handle{new constant{t}};
                }
                template<class T>
                auto to_expr(T const& t)->decltype(t.to_expr())
                {
                        return t.to_expr();
                }
        }
        
        template<class... Args>
        auto call_(std::string const& name, Args&&... args){
                return frontend_wrapper{expr::handle{new call{name, detail::to_expr(args)...}}};
        }

        template<class L, class R>
        auto operator+(L const& l, R const& r)
                ->decltype( frontend_wrapper{expr::handle{new operator_{"+", detail::to_expr(l), detail::to_expr(r)}}})
        {
                return frontend_wrapper{expr::handle{new operator_{"+", detail::to_expr(l), detail::to_expr(r)}}};
        }
        template<class L, class R>
        auto operator-(L const& l, R const& r)
                ->decltype( frontend_wrapper{expr::handle{new operator_{"-", detail::to_expr(l), detail::to_expr(r)}}})
        {
                return frontend_wrapper{expr::handle{new operator_{"-", detail::to_expr(l), detail::to_expr(r)}}};
        }

        namespace literal{
                inline
                frontend_wrapper operator "" _s(const char* name){
                        return sym(name);
                }
                inline
                frontend_wrapper operator "" _c(unsigned long long int val){
                        return const_(val);
                }
        }

} // frontend
} // calg

