#define BOOST_TEST_MODULE TEST_DATA_FRAME_VIEW
#define BOOST_TEST_DYN_LINK
#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/storage.hpp>
#include <boost/test/unit_test.hpp>
#include "data_frame.hpp"
#include <vector>
#include <iostream>
#include <tuple>
#include <typeinfo>
#include <type_traits>
#include <string>
using namespace boost::numeric::ublas;
using namespace std::string_literals;
BOOST_AUTO_TEST_SUITE(test_data_frame_view)

BOOST_AUTO_TEST_CASE(data_frame_view_range_init) {
    using type_collection = type_list<double, long>::types;
    data_frame df4(type_collection{});
    df4.from_tuples(std::vector{std::make_tuple(3.3, 10L), 
                                std::make_tuple(2.2, 40L), 
                                std::make_tuple(1.1, 50L)}, 
                    {"double_vec", "long_vec"});
    range r(2, 3);
    auto cur_view = df4.create_view_with_range(r);
    BOOST_CHECK_EQUAL(cur_view.get_cur_rows(), 1);
    BOOST_CHECK_EQUAL(cur_view.get_cur_cols(), 2);
    cur_view.print_with_index({0});
}
BOOST_AUTO_TEST_CASE(data_frame_view_select_test) {
    using type_collection = type_list<double, long>::types;
    data_frame df4(type_collection{});
    df4.from_tuples(std::vector{std::make_tuple(3.3, 10L), 
                                std::make_tuple(2.2, 40L), 
                                std::make_tuple(1.1, 50L)}, 
                    {"double_vec", "long_vec"});
    auto cur_view = df4.select<long>("long_vec", [](long curVal) {
        return curVal >= 40;
    });
    BOOST_CHECK_EQUAL(cur_view.get_cur_rows(), 2);
    BOOST_CHECK_EQUAL(cur_view.get_cur_cols(), 2);
    cur_view.print_with_index({0, 1});
}
BOOST_AUTO_TEST_CASE(data_frame_apply_with_index_test) {
    using type_collection = type_list<double, long>::types;
    data_frame df4(type_collection{});
    df4.from_tuples(std::vector{std::make_tuple(3.3, 10L), 
                                std::make_tuple(2.2, 40L), 
                                std::make_tuple(1.1, 50L)}, 
                    {"double_vec", "long_vec"});
    auto cur_view = df4.select<long>("long_vec", [](long curVal) {
                            return curVal >= 40;
                        })
                        .apply_with_index({0, 1}, [](auto& t) {
                            return t * 2;
                        });
    auto& cur_val = cur_view.get<long>("long_vec"s, 0);
    BOOST_CHECK_EQUAL(cur_val, 20);
    cur_view.print_with_index({0, 1});   
}
BOOST_AUTO_TEST_CASE(data_frame_sort_with_index_test) {
    using type_collection = type_list<double, long>::types;
    data_frame df4 = type_collection{};
    df4.from_tuples(std::vector{std::make_tuple(3.3, 10L), 
                                std::make_tuple(2.2, 40L), 
                                std::make_tuple(1.1, 50L)}, 
                    {"double_vec", "long_vec"});
    auto cur_view = df4.select<long>("long_vec", [](long curVal) {
                            return curVal >= 40;
                        })
                        .apply_with_index({0, 1, 2}, [](auto& t) {
                            return t * 2;
                        });
    auto& new_view = cur_view.sort<long>("long_vec");
    BOOST_CHECK_EQUAL(new_view.get<long>("long_vec", 0), 20);
    BOOST_CHECK_EQUAL(new_view.get<long>("long_vec", 1), 80);
    BOOST_CHECK_EQUAL(new_view.get<double>("double_vec", 0), 6.6);
    auto& new_view2 = cur_view.sort<double>("double_vec");
    // This sort function is still not correct
    BOOST_CHECK_EQUAL(new_view.get<double>("double_vec", 0), 6.6);
    BOOST_CHECK_EQUAL(new_view.get<double>("double_vec", 1), 4.4);
}
BOOST_AUTO_TEST_SUITE_END()