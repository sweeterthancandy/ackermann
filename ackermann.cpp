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

	enum kind{
		kind_constant,
		kind_symbol,
		kind_call,
		kind_operator,
		sizeof_kind
	};

        virtual ~expr()=default;
        virtual std::ostream& dump(std::ostream&)const=0;
	virtual expr* clone()const=0;
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
	expr* clone()const override{
		return new constant{val_};
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
	expr* clone()const override{
		return new symbol{sym_};
	}
	kind get_kind()const override{
		return kind_symbol;
	}
private:
        std::string sym_;
};

struct call : expr{
        using args_vector = std::vector<expr*>;

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
	expr* clone()const override{
		return new call{name_, args_};
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
	explicit  operator_(std::string const& op, expr* arg0): call{op, args_vector{arg0}}{}
	explicit  operator_(std::string const& op, expr* arg0, expr* arg1): call{op, args_vector{arg0, arg1}}{}

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
	expr* clone()const override{
		return new operator_{get_name(), get_args()};
	}
	kind get_kind()const override{
		return kind_operator;
	}
};


namespace transforms{
        struct constant_folding{
                bool operator()(expr*& root)const{
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
						expr::value_type arg0{ reinterpret_cast<constant*>(ptr->get_arg(0))->get_value()};
						expr::value_type arg1{ reinterpret_cast<constant*>(ptr->get_arg(1))->get_value()};
						
						if( ptr->get_name() == "+" ){
							root = new constant(arg0 + arg1);
							changed = true;
						} else if( ptr->get_name() == "-" ){
							root = new constant(arg0 - arg1);
							changed = true;
						}
					}
				}

			}

			return changed;
                }
        };
}

/*
		         / y +1                 if x = 0
		A(x,y) = | A(x-1, 1)            if x > 0 and y = 0
                         \ A(x-1, A(x,y-1))     if x > 0 and y > 0
*/
struct ackermann_function{
        bool operator()(expr*& root)const{
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
						root = new operator_("+", ptr->get_arg(1)->clone(), new constant(1));
						return true;
					} else if( ptr->get_arg(1)->get_kind() == expr::kind_constant ){
						auto y{ reinterpret_cast<constant*&>( ptr->get_arg(1)) };
						if( y->get_value() == 0 ){
							// A(x-1, 1)
							root = new call("A", 
									new operator_("-", x->clone(), new constant(1)),
									new constant(1));
							return true;
						} else {
							// A(x,n) = A(x-1, A(x, n-1))
							root = new call("A", 
									new operator_("-", x->clone(), new constant(1)),
									new call("A", 
										x->clone(),
										new operator_("-", y->clone(), new constant(1))));
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
	bool operator()(expr*& root)const{
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
	void push(std::string const& sym, expr* e){
		m_[sym] = e;
	}
private:
	std::map<std::string, expr*> m_;
};

struct eval_context{
};

void eval(eval_context& ctx, expr*& root){
        //std::cout << *root << "\n";
        for(;;){
                bool changed = false;
                bool result = false;
                result = transforms::constant_folding()(root);
                changed = changed || result;
                result =  ackermann_function()(root);
                changed = changed || result;
                //std::cout << *root << "\n";
                if( ! changed )
                        break;
        }
}

int ackermann(int x, int y){
        call::args_vector args; 
        args.push_back( new constant(x));
        args.push_back( new constant(y));
        expr* root = new call("A", args);

        eval_context ctx;
        eval(ctx, root);

        if( root->get_kind() == expr::kind_constant ){
                return reinterpret_cast<constant*>(root)->get_value();
        }
        std::cout << *root << "\n";
        throw std::domain_error("doesn't eval to int");
}

#include <gtest/gtest.h>

TEST( algebra_ackermann, low){

        #define _(x,y,r) EXPECT_EQ( r, ackermann(y,x) );

        /* m\n        0          1          2         3          4 */
        /*  0  */ _(0,0,1)  _(1,0,2)   _(2,0,3)    _(3,0,4)    _(4,0,5)
        /*  1  */ _(0,1,2)  _(1,1,3)   _(2,1,4)    _(3,1,5)    _(4,1,6)
       
        /*  2  */ _(0,2,3)  _(1,2,5)    _(2,2,7)    _(3,2,9)    _(4,2,11)
        /*  3  */ _(0,3,5)  _(1,3,13)   _(2,3,29)   _(3,3,61)   _(4,3,125)
}
