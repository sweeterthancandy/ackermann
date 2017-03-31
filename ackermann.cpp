#include "calg/calg.h"

#if 0
#include <cassert>
void other_test(){
        #define _(x,y,r) do{ auto ret{ackermann(x,y)}; std::cout << "A(" << x << ", " << y << ") = " << ret << "    "  << ( r == ret ? "OK" : "FAILED" ) << "\n"; }while(0);
        /* m\n        0          1          2         3          4 */
        /*  0  */ _(0,0,1)  _(0,1,2)   _(0,2,3)    _(0,3,4)    _(0,4,5)
        /*  1  */ _(1,0,2)  _(1,1,3)   _(1,2,4)    _(1,3,5)    _(1,4,6)
        /*  2  */ _(2,0,3)  _(2,1,5)   _(2,2,7)    _(2,3,9)    _(2,4,11)
        /*  3  */ _(3,0,5)  _(3,1,13)  _(3,2,29)   _(3,3,61)   _(3,4,125)  // _(3,5,253)   _(3,6,509) _(3,7,1021)
        #undef _
}

void plus_folding_test(){
        using match_dsl::match;
        using match_dsl::_;
        using match_dsl::_c;
        using match_dsl::_s;

        using frontend::const_;
        using frontend::sym;
        using frontend::call_;
        using namespace frontend::literal;



        factory fac;
        transforms::plus_folding pf;


        expr::handle expr;

        auto t = [&](expr::handle h){
                pf(h);
                return h;
        };
                
        auto a = sym("a");
        auto b = sym("b");
        auto c = sym("c");

        PRINT_SEQ((1_c));
        PRINT_SEQ((1_c+2));
        PRINT_SEQ((*t(1_c)));
        PRINT_SEQ((*t(1_c+2)));

        PRINT_TEST( match(  1_c , _c ) );
        PRINT_TEST( match( t(1_c), _c ) );
        PRINT_TEST( match( t(1_c), _c(1) ) );
        
        PRINT_TEST( match(  1_c + 2  , _c + _c ));
        PRINT_TEST( match(  1_c + 2  , _c(1) + _c(2) ));
        
        PRINT_TEST( match(t(1_c + 2)  , _c ) ) ;
        PRINT_TEST( match(t(1_c + 2)  , _c(3) ) );
        
        PRINT_TEST( match(t(a + b + 1)  , _s + _s + _c(1) ) );
        PRINT_TEST( match(t(a + 1 + 2)  , _s + _c(3) ) );
        PRINT_TEST( match(t(1_c + 2 + 3)  , _c(6) ) );
        PRINT_TEST( match(t(1_c + a + 3)  , _c(4) + _s ) );
        PRINT_SEQ((*t(1_c + a + 3)));

        //PRINT_TEST( match(t(a + b + c)  , _s + _s + _s ) );



}

void github_render(){
        factory fac;
        auto root{ fac.call("A", fac.constant(3), fac.constant(2)) };

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
        //github_render();
        //other_test();
        plus_folding_test();
}
#endif
int main(){}
         
// vim: sw=8
