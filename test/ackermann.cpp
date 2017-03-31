
#include <gtest/gtest.h>

#include "calg/calg.h"

using namespace calg;

int ackermann(int x, int y){
        call::args_vector args; 
        factory fac;
        args.push_back( fac.constant(x));
        args.push_back( fac.constant(y));
        auto root{ fac.call("A", args)};

        eval_context ctx;
        ctx
                .push(transform::constant_folding())
                .push(transform::ackermann_function())
                .push(transform::constant_folding())
                .push(transform::plus_folding());
        eval(ctx, root);

        if( root->get_kind() == expr::kind_constant ){
                return reinterpret_cast<constant*>(root.get())->get_value();
        }
        std::cout << *root << "\n";
        std::exit(1);
        //throw std::domain_error("doesn't eval to int");
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
