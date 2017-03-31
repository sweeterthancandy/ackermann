#pragma once

namespace calg{

namespace detail{

        enum numeric_group{
                numeric_group_none,
                numeric_group_addition,
                numeric_group_multiplication
        };

        enum commutivity{
                does_not_commute,
                does_commute,
                anti_commute
        };
                
        struct operator_traits_t{
                numeric_group group;
                int precedence;
                commutivity commute;
        };

        static
        std::map<std::string, operator_traits_t> operator_traits = {
                { "+", {numeric_group_addition      , 0, does_commute } },
                { "-", {numeric_group_addition      , 0, anti_commute} } ,
                { "*", {numeric_group_multiplication, 1, does_commute} } ,
                { "/", {numeric_group_multiplication, 1, anti_commute} } ,
                { "%", {numeric_group_none          , 3, does_not_commute} }
        };

} // detail

} // calg
