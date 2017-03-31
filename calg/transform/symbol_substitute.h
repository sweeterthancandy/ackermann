#pragma once

namespace calg{
namespace transform{


struct symbol_substitute{
	bool operator()(expr::handle& root)const{
                return apply_bottom_up( [this](expr::handle& root)->bool{
                        using match_dsl::match;
                        using match_dsl::_s;

                        if( match( root, _s ) ){
                                auto ptr{ reinterpret_cast<symbol*>(root.get()) };
                                auto mapped{m_.find(ptr->get_name())};
                                if( mapped != m_.end()){
                                        root = mapped->second->clone();
                                        return true;
                                }
                        }
                        return false;
                }, root);
	}
	void push(std::string const& sym, expr::handle e){
		m_[sym] = e;
	}
private:
	std::map<std::string, expr::handle> m_;
};


} // calg
} // transform

