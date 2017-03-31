#include <gtest/gtest.h>

#include "calg/calg.h"
        
using namespace calg;
using match_dsl::match;
using match_dsl::_;
using match_dsl::_c;
using match_dsl::_s;

using frontend::const_;
using frontend::sym;
using frontend::call_;
using namespace frontend::literal;


struct match_ : public testing::Test{
        virtual void SetUp()override {
                a = sym("a");
                b = sym("b");
                _1 = const_(1);
                _2 = const_(2);
        }
        frontend::frontend_wrapper a, b, _1, _2;
};


TEST_F( match_, __){
        EXPECT_TRUE( match( a         , _ ) );
        EXPECT_TRUE( match( _1        , _ ) );
        EXPECT_TRUE( match( a + _1    , _ ));
        EXPECT_TRUE( match( _1 - _1   , _ ));
        EXPECT_TRUE( match( _1 - a + b, _ ));
}

TEST_F( match_, _c_){
        EXPECT_FALSE( match( a         , _c ) );
        EXPECT_TRUE( match( _1         , _c ) );
        EXPECT_TRUE( match( _1         , _c(1) ) );
        EXPECT_FALSE( match( _1        , _c(0) ) );
        EXPECT_FALSE( match( a + _1    , _c ));
        EXPECT_FALSE( match( _1 - _1   , _c ));
        EXPECT_FALSE( match( _1 - a + b, _c ));
}

TEST_F( match_, _s_){
        EXPECT_TRUE( match( a          , _s ) );
        EXPECT_TRUE( match( a          , _s("a") ) );
        EXPECT_FALSE( match( a         , _s("aa") ) );

        EXPECT_FALSE( match( _1        , _s ) );
        EXPECT_FALSE( match( _1         , _s ) );
        EXPECT_FALSE( match( _1        , _s ) );
        EXPECT_FALSE( match( a + _1    , _s ));
        EXPECT_FALSE( match( _1 - _1   , _s ));
        EXPECT_FALSE( match( _1 - a + b, _s ));
}

