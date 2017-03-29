#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <numeric>
#include <memory>
#include <type_traits>

#define BOOST_NO_EXCEPTIONS

#include <boost/range/algorithm.hpp>
#include <boost/preprocessor.hpp>
#include <boost/function.hpp>
#include <boost/functional/hash.hpp>
#include <boost/fusion/include/vector.hpp>
#include <boost/fusion/include/algorithm.hpp>

#define PRINT_SEQ_detail(r, d, i, e) do{ std::cout << ( i ? ", " : "" ) << BOOST_PP_STRINGIZE(e) << " = " << (e); }while(0);
#define PRINT_SEQ(SEQ) do{ BOOST_PP_SEQ_FOR_EACH_I( PRINT_SEQ_detail, ~, SEQ) std::cout << "\n"; }while(0)
#define PRINT_TEST( EXPR ) do{ auto ret{EXPR}; std::cout << #EXPR << ( ret ? "OK" : "FAILED" ) << "\n"; }while(0);

namespace boost{
        void throw_exception( std::exception const & e ){
                std::cerr << e.what();
                std::exit(1);
        }
}

struct expr{
        using value_type = int;
        using handle = std::shared_ptr<expr>;

	enum kind{
                kind_begin_terminal,
                        kind_constant,
                        kind_symbol,
                kind_end_terminal,
		kind_call,
		kind_operator,
		sizeof_kind
	};

        virtual ~expr()=default;
        virtual std::ostream& dump(std::ostream&)const=0;
	virtual handle clone()const=0;
	virtual kind get_kind()const=0; 
        /*
        
           1) want to hash the full expression
                        1+2 ~ 1+2
           2) want to hash the expression type
                        1+2 ~ 4+5
           2) want to match function call type
                        f(x) ~ f(2+3)
           3) want to match 

         */
        size_t get_type_hash()const{
                size_t seed{0};
                this->type_hash(seed);
                return seed;
        }
        size_t get_value_hash()const{
                size_t seed{0};
                this->value_hash(seed);
                return seed;
        }
        size_t get_hash()const{
                size_t seed{0};
                this->type_hash(seed);
                this->value_hash(seed);
                return seed;
        }
        // default impl
        virtual void type_hash(size_t& seed)const{
                boost::hash_combine(seed, get_kind());
        }
        virtual void value_hash(size_t& seed)const{
        }
        bool is_terminal()const{
                return kind_begin_terminal < get_kind() && get_kind() <= kind_end_terminal;
        }


        friend std::ostream& operator<<(std::ostream& ostr, expr const& e){
                return e.dump(ostr);
        }
};




struct constant : expr{
        explicit constant(value_type val):val_(val){}
        value_type const& get_value()const{ return val_; }
        value_type& get_value(){ return val_; }
        virtual std::ostream& dump(std::ostream& ostr)const override{
                return ostr << val_;
        }
	handle clone()const override{
		return handle{new constant{val_}};
	}
	kind get_kind()const override{
		return kind_constant;
	}
        void value_hash(size_t& seed)const override{
                boost::hash_combine(seed, val_);
        }
private:
        value_type val_;
};

struct symbol : expr{
        explicit symbol(std::string const& sym):sym_(sym){}
        decltype(auto) get_name()const{ return sym_; }
        virtual std::ostream& dump(std::ostream& ostr)const override{
                return ostr << sym_;
        }
	handle clone()const override{
		return handle{new symbol{sym_}};
	}
	kind get_kind()const override{
		return kind_symbol;
	}
        void value_hash(size_t& seed)const override{
                boost::hash_combine(seed, sym_);
        }
private:
        std::string sym_;
};

struct non_terminal_expr : expr{
        using args_vector = std::vector<handle>;

        template<class... Args>
        explicit non_terminal_expr(Args&&... args) : args_{std::forward<Args>(args)...}{}
	
        decltype(auto) arg_begin()const{ return args_.begin(); }
	decltype(auto) arg_end()const{ return args_.end(); }
	decltype(auto) arg_begin(){ return args_.begin(); }
	decltype(auto) arg_end(){ return args_.end(); }
	decltype(auto) get_arg(size_t idx)const{ return args_[idx]; }
	decltype(auto) get_arg(size_t idx)     { return args_[idx]; }
	decltype(auto) get_args()const{ return args_; }
	decltype(auto) get_args()     { return args_; }
private:
	args_vector args_;
};

struct call : non_terminal_expr{

	explicit call(std::string const& name, args_vector const& args): non_terminal_expr{args}, name_{name}{}

	template<class... Args>
	explicit call(std::string const& name, Args const&... args):non_terminal_expr{args...},name_{name}{}

        
	decltype(auto) get_name()const{ return name_; }


	auto get_arity()const { return get_args().size(); }

        
	virtual std::ostream& dump(std::ostream& ostr)const override{
		ostr << name_ << "(";
		bool comma{false};
		for( auto& arg : get_args()){
			ostr << ( comma ? "," : "" );
			arg->dump(ostr);
			comma = true;
		}
		ostr << ")";
		return ostr;
	}
	handle clone()const override{
		return handle{new call{name_, get_args()}};
	}
	kind get_kind()const override{
		return kind_call;
	}
        void value_hash(size_t& seed)const override{
                boost::hash_combine(seed, name_);
                for( auto const& arg : get_args() )
                        arg->value_hash(seed);
        }
        void type_hash(size_t& seed)const override{
                expr::type_hash(seed);
                boost::hash_combine(seed, name_ );
                boost::hash_combine(seed, get_args().size() );
                #if 0
                for( auto const& arg : get_args() )
                        arg->type_hash(seed);
                #endif
        }
private:
	std::string name_;
};

struct operator_ : call{
	explicit  operator_(std::string const& op, args_vector const& args): call{op, args}{}
	explicit  operator_(std::string const& op, handle arg0): call{op, args_vector{arg0}}{}
	explicit  operator_(std::string const& op, handle arg0, handle arg1): call{op, args_vector{arg0, arg1}}{}

	virtual std::ostream& dump(std::ostream& ostr)const override{
		if( get_arity() == 2 ){
			get_arg(0)->dump(ostr);
			ostr << get_name();
			get_arg(1)->dump(ostr);
		} else if( get_arity() == 1 ){
			ostr << get_name();
			get_arg(0)->dump(ostr);
	 	} else{
			return call::dump(ostr);
		}
		return ostr;
	}
	handle clone()const override{
		return handle{new operator_{get_name(), get_args()}};
	}
	kind get_kind()const override{
		return kind_operator;
	}
};


namespace match_dsl{

        struct any_matcher{
                bool match(expr::handle)const{
                        return true;
                }
                auto __to_matcher__()const{ return *this; }
        };

        struct kind_matcher{
                explicit kind_matcher(expr::kind k):k_(k){}
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
                template<class... Args>
                auto operator()(std::string const& name, Args&&... args)const{
                        return call_matcher<std::decay_t<Args>...>(name, std::forward<Args>(args)...);
                }
                auto __to_matcher__()const{ return *this; }
        private:
                std::string name_;
                args_t args_;
        };


        template<class L, class R>
        struct operator_matcher{
                explicit operator_matcher(std::string const& name, L const& l, R const& r):
                        name_(name), l_(l), r_(r)
                {}
                bool match(expr::handle expr)const{
                        if( expr->get_kind() != expr::kind_operator )
                                return false;
                        auto op{ reinterpret_cast<operator_*>(expr.get())};
                        if( op->get_name() != name_ || op->get_arity() != 2 )
                                return false;
                        return l_.match( op->get_arg(0) ) && r_.match( op->get_arg(1) );
                }
                auto __to_matcher__()const{ return *this; }
        private:
                std::string name_;
                L l_;
                R r_;
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


        auto _ = any_matcher{};
        auto _c = constant_matcher{};
        auto _call = call_matcher<>{};

}


struct factory{
        using handle = expr::handle;

        auto constant(expr::value_type val){
                auto iter{constant_map_.find(val)};
                if( iter == constant_map_.end()){
                        expr::handle ret{ new ::constant(val)};
                        constant_map_[val] = ret;
                        return ret;
                }
                return iter->second;
        }
        template<class... Args>
        auto operator_(Args&&... args){
                return expr::handle{ new ::operator_(std::forward<Args>(args)...)};
        }
        template<class... Args>
        auto call(Args&&... args){
                return expr::handle{ new ::call(std::forward<Args>(args)...)};
        }
private:
        std::map< int, expr::handle> constant_map_;
};

namespace matching{
}


/*                  +                    +    
                  /   \                /    \
                +       1   ->      alpha    2
              /    \
           alpha    2
 
 */

template<class F>
bool apply_bottom_up(F f, expr::handle& root){
        std::vector<expr::handle*> stack{&root};
        for(;stack.size();){
                auto ptr{stack.back()};
                stack.pop_back();


        }
}

namespace transforms{
        struct plus_folding{
                bool operator()(expr::handle& root)const{
                        using match_dsl::_;
                        using match_dsl::_c;
                        using match_dsl::match;
			bool changed{false};
                        std::vector<expr::handle*> stack{&root};


                        //std::cout << "root = " << *root << "\n";

                        for(; stack.size(); ){
                                expr::handle& ptr{*stack.back()};
                                stack.pop_back();

                                //PRINT_SEQ((stack.size()));

                                if( match( ptr, _ + _ ) ){
                                        // need to find the non-leaf chidlren
                                        std::vector<expr::handle*> sub_stack{&ptr};
                                        std::vector<expr::handle*> args;

                                        //std::cout << "  found " << *ptr << std::endl;

                                        for(;sub_stack.size();){
                                                auto& sub{*sub_stack.back()};
                                                sub_stack.pop_back();
                                                auto as_op{ reinterpret_cast<operator_*>(sub.get())};


                                                if( match( sub, _ + _ ) )
                                                {
                                                        //std::cout << "    found sub " << *sub << "\n";
                                                        for( auto iter{as_op->arg_begin()}, end{as_op->arg_end()}; iter!=end;++iter){
                                                                sub_stack.emplace_back( &*iter );
                                                        }
                                                } else{
                                                        //std::cout << "    found arg " << *sub << "\n";
                                                        // leaf
                                                        args.emplace_back(&sub);
                                                }
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
        #if 1
        struct constant_folding{
                bool operator()(expr::handle& root)const{
			bool changed{false};
			switch( root->get_kind()){
			case expr::kind_operator:
			case expr::kind_call: {
				auto ptr{ reinterpret_cast<call*&>(root) };
				for( auto iter{ptr->arg_begin()}, end{ptr->arg_end()}; iter!=end;++iter){
					bool result{(*this)(*iter)};
					changed = changed || result;
				}
			}
				break;
			default:
				break;
			}

                        using match_dsl::_c;
                        using match_dsl::match;
                        if( match( root, _c + _c || _c - _c ) ){
                                auto ptr{ reinterpret_cast<operator_*&>(root) };
                                expr::value_type arg0{ reinterpret_cast<constant*>(ptr->get_arg(0).get())->get_value()};
                                expr::value_type arg1{ reinterpret_cast<constant*>(ptr->get_arg(1).get())->get_value()};

                                if( ptr->get_name() == "+" ){
                                        root = expr::handle{new constant(arg0 + arg1)};
                                        changed = true;
                                } else if( ptr->get_name() == "-" ){
                                        root = expr::handle{new constant(arg0 - arg1)};
                                        changed = true;
                                }
                        }

			return changed;
                }
        };
        #endif
}


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
		bool changed{false};
                        
                using match_dsl::_;
                using match_dsl::_c;
                using match_dsl::_call;
                using match_dsl::match;


		switch( root->get_kind()){
		case expr::kind_operator:
		case expr::kind_call: {
			auto ptr{ reinterpret_cast<call*&>(root) };
			for( auto iter{ptr->arg_begin()}, end{ptr->arg_end()}; iter!=end;++iter){
				bool result{(*this)(*iter)};
				changed = changed || result;
			}
		}
			break;
		default:
			break;
		}


/*
		         / y +1                 if x = 0
		A(x,y) = | A(x-1, 1)            if x > 0 and y = 0
                         \ A(x-1, A(x,y-1))     if x > 0 and y > 0
*/
                if( match( root, _call("A", _c(0), _) ) ){
                        auto c{ reinterpret_cast<call*>( root.get()) };
                        root = fac.operator_("+", c->get_arg(1), _1);
                        return true;
                } else if( match( root, _call("A", _c, _c ) ) ){
                        auto c{ reinterpret_cast<call*>( root.get()) };
                        auto x{ reinterpret_cast<constant*>( c->get_arg(0).get()) };
                        auto y{ reinterpret_cast<constant*>( c->get_arg(1).get()) };

                        if( y->get_value() == 0 ){
                                // A(x-1, 1)
                                root = fac.call("A", 
                                                fac.operator_("-", c->get_arg(0), _1),
                                                _1);
                                return true;
                        } else {
                                // A(x,n) = A(x-1, A(x, n-1))
                                root = fac.call("A", 
                                                fac.operator_("-", c->get_arg(0), _1),
                                                fac.call("A", 
                                                         c->get_arg(0),
                                                         fac.operator_("-", c->get_arg(1), _1)));
                                return true;
                        }
                }

		return changed;
        }
};

struct symbol_substitute{
	bool operator()(expr::handle& root)const{
		bool changed{false};
		switch( root->get_kind()){
		case expr::kind_call: {
			auto ptr{ reinterpret_cast<call*&>(root) };
			for( auto iter{ptr->arg_begin()}, end{ptr->arg_end()}; iter!=end;++iter){
				bool result{(*this)(*iter)};
				changed = changed || result;
			}
		}
			break;
		case expr::kind_symbol: {
			auto ptr{ reinterpret_cast<symbol*&>(root) };
			auto mapped{m_.find(ptr->get_name())};
			if( mapped != m_.end()){
				root = mapped->second->clone();
				changed = true;
			}
		}
			break;
		default:
			break;
		}
		return changed;
	}
	void push(std::string const& sym, expr::handle e){
		m_[sym] = e;
	}
private:
	std::map<std::string, expr::handle> m_;
};

struct eval_context{
        using erased_transform_t = boost::function<bool(expr::handle&)>;
        eval_context&  push(erased_transform_t t){
                transforms_.push_back(std::move(t));
                return *this;
        }
        auto begin(){ return transforms_.begin(); }
        auto end(){ return transforms_.end(); }
private:
        std::vector<erased_transform_t> transforms_;
};

bool eval_once(eval_context& ctx, expr::handle& root){
        bool changed = false;
        for( auto& t : ctx ){
                bool result{t(root)};
                changed = changed || result;
        }
        return changed; 
}


void eval(eval_context& ctx, expr::handle& root){
        for(;;){
                bool changed{eval_once(ctx, root)};
                if( ! changed )
                        break;
        }
}

int ackermann(int x, int y){
        call::args_vector args; 
        factory fac;
        args.push_back( fac.constant(x));
        args.push_back( fac.constant(y));
        auto root{ fac.call("A", args)};

        eval_context ctx;
        ctx
                .push(transforms::constant_folding())
                .push(ackermann_function())
                .push(transforms::constant_folding())
                .push(transforms::plus_folding());
        eval(ctx, root);

        if( root->get_kind() == expr::kind_constant ){
                return reinterpret_cast<constant*>(root.get())->get_value();
        }
        std::cout << *root << "\n";
        std::exit(1);
        //throw std::domain_error("doesn't eval to int");
}

#if 0
#include <gtest/gtest.h>

#define _(x,y,r) EXPECT_EQ( r, ackermann(x,y) );
TEST( algebra_ackermann, low){


        /* m\n        0          1          2         3          4 */
        /*  0  */ _(0,0,1)  _(0,1,2)   _(0,2,3)    _(0,3,4)    _(0,4,5)
        /*  1  */ _(1,0,2)  _(1,1,3)   _(1,2,4)    _(1,3,5)    _(1,4,6)
        /*  2  */ _(2,0,3)  _(2,1,5)   _(2,2,7)    _(2,3,9)    _(2,4,11)
        /*  3  */ _(3,0,5)  _(3,1,13)  _(3,2,29)   _(3,3,61)   _(3,4,125)  _(3,5,253)   _(3,6,509) _(3,7,1021)

}

TEST( algebra_ackermann, 4_x){
        _(4,0, 13)
        _(4,1, 65'533)
}




#undef _
#endif


#include <cassert>
void other_test(){
        #define _(x,y,r) do{ auto ret{ackermann(x,y)}; std::cout << "A(" << x << ", " << y << ") = " << ret << "    "  << ( r == ret ? "OK" : "FAILED" ) << "\n"; }while(0);
        /* m\n        0          1          2         3          4 */
        /*  0  */ _(0,0,1)  _(0,1,2)   _(0,2,3)    _(0,3,4)    _(0,4,5)
        /*  1  */ _(1,0,2)  _(1,1,3)   _(1,2,4)    _(1,3,5)    _(1,4,6)
        /*  2  */ _(2,0,3)  _(2,1,5)   _(2,2,7)    _(2,3,9)    _(2,4,11)
        /*  3  */ _(3,0,5)  _(3,1,13)  _(3,2,29)   _(3,3,61)   _(3,4,125)  _(3,5,253)   _(3,6,509) _(3,7,1021)
        #undef _
}

void github_render(){
        factory fac;
        auto root{ fac.call("A", fac.constant(3), fac.constant(3)) };

        eval_context ctx;
        ctx
                .push(transforms::constant_folding())
                .push(ackermann_function())
                .push(transforms::constant_folding())
                .push(transforms::plus_folding())
        ;

        std::cout << *root << "\n";
        for(;;){
                auto changed{ eval_once(ctx, root) };
                if( ! changed )
                        break;
                std::cout << *root << "\n";
        }
}

int main(){
        other_test();
}
         
// vim: sw=8
