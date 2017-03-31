#pragma once

namespace calg{


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
        bool is_non_terminal()const{
                return ! is_terminal();
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


} // calg

