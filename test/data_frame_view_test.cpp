#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/storage.hpp>
#include "data_frame.hpp"
#include <vector>
#include <iostream>
#include <tuple>
#include <typeinfo>
#include <type_traits>
#include <string>
#include "gtest/gtest.h"
#include "data_frame.hpp"
using namespace boost::numeric::ublas;
using namespace std::string_literals;
TEST(Data_frame_view, range_init) {
    using type_collection = type_list<double, long>::types;
    data_frame df4;
    df4.from_tuples(std::vector{std::make_tuple(3.3, 10L), 
                                std::make_tuple(2.2, 40L), 
                                std::make_tuple(1.1, 50L)}, 
                    {"double_vec", "long_vec"});
    range r(2, 3);
    auto cur_view = df4.create_view_with_range(r, type_collection{});
    EXPECT_EQ(cur_view.get_cur_rows(), 1);
    EXPECT_EQ(cur_view.get_cur_cols(), 2);
    cur_view.print_with_index({0});
    auto cur_view2 = df4.select<std::string>("str_vec", [](std::string curVal) {
        return curVal >= "asdjlj";
    }, type_collection{});
    EXPECT_EQ(cur_view2.get_cur_rows(), 0);
    EXPECT_EQ(cur_view2.get_cur_cols(), 2);
}
TEST(Data_frame_view, select_test) {
    using type_collection = type_list<double, long>::types;
    data_frame df4;
    df4.from_tuples(std::vector{std::make_tuple(3.3, 10L), 
                                std::make_tuple(2.2, 40L), 
                                std::make_tuple(1.1, 50L)}, 
                    {"double_vec", "long_vec"});
    auto cur_view = df4.select<long>("long_vec", [](long curVal) {
        return curVal >= 40;
    }, type_collection{});
    EXPECT_EQ(cur_view.get_cur_rows(), 2);
    EXPECT_EQ(cur_view.get_cur_cols(), 2);
    cur_view.print_with_index({0, 1});
    auto cur_view2 = df4.select<std::string>("str_vec", [](std::string curVal) {
        return curVal >= "asdjlj";
    }, type_collection{});
    EXPECT_EQ(cur_view2.get_cur_rows(), 0);
    EXPECT_EQ(cur_view2.get_cur_cols(), 2);
}
TEST(Data_frame, apply_with_index_test) {
    using type_collection = type_list<double, long>::types;
    data_frame df4;
    df4.from_tuples(std::vector{std::make_tuple(3.3, 10L), 
                                std::make_tuple(2.2, 40L), 
                                std::make_tuple(1.1, 50L)}, 
                    {"double_vec", "long_vec"});
    auto cur_view = df4.select<long>("long_vec", [](long curVal) {
                            return curVal >= 40;
                        }, type_collection{})
                        .apply_with_index({0, 1}, [](auto& t) {
                            return t * 2;
                        });
    auto& cur_val = cur_view.get<long>("long_vec"s, 0);
    EXPECT_EQ(cur_val, 20);
    cur_view.print_with_index({0, 1});   
}
TEST(Data_frame, sort_with_index_test) {
    using type_collection = type_list<double, long>::types;
    data_frame df4;
    df4.from_tuples(std::vector{std::make_tuple(3.3, 10L), 
                                std::make_tuple(2.2, 40L), 
                                std::make_tuple(1.1, 50L)}, 
                    {"double_vec", "long_vec"});
    auto cur_view = df4.select<long>("long_vec", [](long curVal) {
                            return curVal >= 40;
                        }, type_collection{})
                        .apply_with_index({0, 1}, [](auto& t) {
                            return t * 2;
                        });
    auto& new_view = cur_view.sort<long>("long_vec");
    EXPECT_EQ(new_view.get<long>("long_vec", 0), 20);
    EXPECT_EQ(new_view.get<long>("long_vec", 1), 80);
    EXPECT_EQ(new_view.get<double>("double_vec", 0), 6.6);
    auto& new_view2 = cur_view.sort<double>("double_vec");
    // This sort function is still not correct
    EXPECT_EQ(new_view.get<double>("double_vec", 0), 4.4);
    EXPECT_EQ(new_view.get<double>("double_vec", 1), 6.6);
}