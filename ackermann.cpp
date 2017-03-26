#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <numeric>
#include <memory>
#include <boost/range/algorithm.hpp>
#include <boost/preprocessor.hpp>

#define PRINT_SEQ_detail(r, d, i, e) do{ std::cout << ( i ? ", " : "" ) << BOOST_PP_STRINGIZE(e) << " = " << (e); }while(0);
#define PRINT_SEQ(SEQ) do{ BOOST_PP_SEQ_FOR_EACH_I( PRINT_SEQ_detail, ~, SEQ) std::cout << "\n"; }while(0)

struct expr{
        using value_type = int;
        using handle = std::shared_ptr<expr>;

	enum kind{
		kind_constant,
		kind_symbol,
		kind_call,
		kind_operator,
		sizeof_kind
	};

        virtual ~expr()=default;
        virtual std::ostream& dump(std::ostream&)const=0;
	virtual handle clone()const=0;
	virtual kind get_kind()const=0; 

        friend std::ostream& operator<<(std::ostream& ostr, expr const& e){
                return e.dump(ostr);
        }
};



struct constant : expr{
        explicit constant(value_type val):val_(val){}
        auto get_value()const{ return val_; }
        virtual std::ostream& dump(std::ostream& ostr)const override{
                return ostr << val_;
        }
	handle clone()const override{
		return handle{new constant{val_}};
	}
	kind get_kind()const override{
		return kind_constant;
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
private:
        std::string sym_;
};

struct call : expr{
        using args_vector = std::vector<handle>;

	explicit call(std::string const& name, args_vector const& args):name_(name),args_(args){}

	template<class... Args>
	explicit call(std::string const& name, Args const&... args):name_(name),args_{args...}{}


        
	decltype(auto) get_name()const{ return name_; }

	auto arg_begin()const{ return args_.begin(); }
	auto arg_end()const{ return args_.end(); }
	auto arg_begin(){ return args_.begin(); }
	auto arg_end(){ return args_.end(); }

	auto get_arity()const { return args_.size(); }

	decltype(auto) get_arg(size_t idx)const{ return args_[idx]; }
	decltype(auto) get_arg(size_t idx)     { return args_[idx]; }
	
	decltype(auto) get_args()const{ return args_; }
	decltype(auto) get_args()     { return args_; }
        
	virtual std::ostream& dump(std::ostream& ostr)const override{
		ostr << name_ << "(";
		bool comma{false};
		for( auto& arg : args_){
			ostr << ( comma ? "," : "" );
			arg->dump(ostr);
			comma = true;
		}
		ostr << ")";
		return ostr;
	}
	handle clone()const override{
		return handle{new call{name_, args_}};
	}
	kind get_kind()const override{
		return kind_call;
	}
private:
	std::string name_;
	args_vector args_;
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


/*                  +                    +    
                  /   \                /    \
                +       1   ->      alpha    2
              /    \
           alpha    2
 
 */
namespace transforms{
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

			if( root->get_kind() == expr::kind_operator ){
				auto ptr{ reinterpret_cast<operator_*&>(root) };
				if( ptr->get_arity() == 2 ){
					if( ptr->get_arg(0)->get_kind() == expr::kind_constant &&
					    ptr->get_arg(1)->get_kind() == expr::kind_constant )
					{
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
				}

			}

			return changed;
                }
        };
}

struct factory{
        using handle = expr::handle;

        #if 0
        template<class... Args>
        auto constant(Args&&... args){
                return expr::handle{ new ::constant(std::forward<Args>(args)...)};
        }
        #endif
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

		switch( root->get_kind()){
		case expr::kind_call: {
			auto ptr{ reinterpret_cast<call*&>(root) };
			if( ptr->get_name() == "A" && ptr->get_arity() == 2){
				// can only expand if it's constant
				if( ptr->get_arg(0)->get_kind() == expr::kind_constant ){
					auto x{ reinterpret_cast<constant*&>( ptr->get_arg(0)) };
					if( x->get_value() == 0 ){
						// n + 1
						root = fac.operator_("+", ptr->get_arg(1), _1);
						return true;
					} else if( ptr->get_arg(1)->get_kind() == expr::kind_constant ){
						auto y{ reinterpret_cast<constant*&>( ptr->get_arg(1)) };
						if( y->get_value() == 0 ){
							// A(x-1, 1)
							root = fac.call("A", 
									fac.operator_("-", ptr->get_arg(0), _1),
									_1);
							return true;
						} else {
							// A(x,n) = A(x-1, A(x, n-1))
							root = fac.call("A", 
									fac.operator_("-", ptr->get_arg(0), _1),
									fac.call("A", 
										ptr->get_arg(0),
										fac.operator_("-", ptr->get_arg(1), _1)));
							return true;
						}
					}
				}
			}
		}
			break;
		default:
			break;
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
};

bool eval_once(eval_context& ctx, expr::handle& root){
        bool changed = false;
        bool result = false;
        result =  ackermann_function()(root);
        changed = changed || result;
        result = transforms::constant_folding()(root);
        changed = changed || result;
        return changed; 
}


void eval(eval_context& ctx, expr::handle& root){
        //std::cout << *root << "\n";
        for(;;){
                bool changed = false;
                bool result = false;
                result =  ackermann_function()(root);
                changed = changed || result;
                result = transforms::constant_folding()(root);
                changed = changed || result;
                //std::cout << *root << "\n";
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
        eval(ctx, root);

        if( root->get_kind() == expr::kind_constant ){
                return reinterpret_cast<constant*>(root.get())->get_value();
        }
        std::cout << *root << "\n";
        throw std::domain_error("doesn't eval to int");
}

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


int main(){
        factory fac;
        auto root{ fac.call("A", fac.constant(2), fac.constant(3)) };

        eval_context ctx;

        std::cout << *root << "\n";
        for(;;){
                auto changed{ eval_once(ctx, root) };
                if( ! changed )
                        break;
                std::cout << *root << "\n";
        }


}
