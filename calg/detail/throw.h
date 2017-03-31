#pragma once

namespace calg{

#ifdef BOOST_NO_EXCEPTIONS
namespace boost{
        void throw_exception( std::exception const & e ){
                std::cerr << e.what();
                std::exit(1);
        }
}
#endif

} // calg
