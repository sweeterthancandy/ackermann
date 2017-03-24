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


#if 0
template<class F>
bool apply_top_down(F f, expr*& root){
	bool changed{false};
	if( call* ptr = dynamic_cast<call*>(root) ){
		f(ptr);
		for( auto iter{ptr->arg_begin()}, end{ptr->arg_end()}; iter!=end;++iter){
			changed = changed || f(*iter);
		}
	} else if ( symbol* ptr = dynamic_cast<symbol*>(root)){
		f( reinterpret_cast<symbol*&>(root) );
	} else{
		// unknown
	}
	return changed;
}
#endif

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
		         / n +1                 if m = 0
		A(m,n) = | A(m-1, 1)            if m > 0 and n = 0
                         \ A(m-1, A(m,n-1))     if m > 0 and n > 0
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
					auto m{ reinterpret_cast<constant*&>( ptr->get_arg(0)) };
					if( m->get_value() == 0 ){
						// n + 1
						root = new operator_("+", ptr->get_arg(1)->clone(), new constant(1));
						return true;
					} else if( ptr->get_arg(1)->get_kind() == expr::kind_constant ){
						auto arg1{ reinterpret_cast<constant*&>( ptr->get_arg(1)) };
						if( arg1->get_value() == 0 ){
							// A(m-1, 1)
							root = new call("A", 
									new operator_("-", m->clone(), new constant(1)),
									new constant(1));
							return true;
						} else {
							// A(m,n) = A(m-1, A(m, n-1))
							root = new call("A", 
									new operator_("-", m->clone(), new constant(1)),
									new call("A", 
										m->clone(),
										new operator_("-", arg1->clone(), new constant(1))));
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

#if 0
struct symbol_substitute_detail{
	bool operator()(symbol*& sym){
		std::cout << "sym\n";
		auto mapped{m_.find(sym->get_name())};
		if( mapped != m_.end()){
			expr* aux{ reinterpret_cast<expr*&>(sym) };
			aux = mapped->second->clone();
		}
		return false;
	}
	template<class F>
	bool operator()(F* ptr){
		std::cout << "default " << ptr << "\n";
		return false;
	}
	void push(std::string const& sym, expr* e){
		m_[sym] = e;
	}
private:
	std::map<std::string, expr*> m_;
};
#endif

#if 1
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
#endif
#if 0
struct symbol_substitute{
	bool operator()(expr*& root)const{
		return apply_top_down( symbol_substitute_detail(), root);
	}
	void push(std::string const& sym, expr* e){
		m_[sym] = e;
	}
private:
	std::map<std::string, expr*> m_;
};
#endif


struct driver{
        void run(){
		call::args_vector args; 
		args.push_back( new symbol("m"));
		args.push_back( new symbol("n"));
		root_ = new call("A", args);
		std::cout << *root_ << "\n";
		symbol_substitute ss;
		ss.push("m", new constant(1));
		ss.push("n", new constant(2));
		ss(root_);
                for(;;){
                        std::cout << *root_ << "\n";
                        bool changed = false;
			bool result = false;
                        result = transforms::constant_folding()(root_);
			if( result )
				std::cout << *root_ << "\n";
			changed = changed || result;
                        result =  ackermann_function()(root_);
			if( result ) 
				std::cout << *root_ << "\n";
			changed = changed || result;
                        if( ! changed )
                                break;
                }
                std::cout << *root_ << "\n";
        }
private:
        expr* root_;
};


int main(){
        try{
                driver dvr;
                dvr.run();
        }
        catch(std::exception& e){
                std::cerr << e.what() << "\n";
                return EXIT_FAILURE;
        }
}
