#include "data_frame_col.hpp"
#include "data_frame.hpp"
using namespace boost::numeric::ublas;
using namespace std::string_literals;
int main() {
    using type_collection = type_list<int, std::string>::types;  
    std::vector<int> int_vec = {0, 1, 2, 3, 4};
    std::vector<std::string> str_vec = {"test0", "test1", "test2", "test3", "test4"};
    data_frame df(type_collection{});
    df.add_column("int_vec", int_vec);
    df.add_column("str_vec", str_vec);
    using type_collection1 = type_list<double, long>::types;
    using type_collection2 = type_list<std::string, int, double>::types;
    data_frame df1 = type_collection1{};
    data_frame df2 = type_collection2{};
    df1.from_tuples(std::vector{std::make_tuple(3.3, 10L), 
                                std::make_tuple(2.2, 40L)}, 
                                {"double_vec", "long_vec"});
    df2.from_tuples(std::vector{std::make_tuple(3.3, "hello"s, 10), 
                                std::make_tuple(3.3, "hello"s, 10),
                                std::make_tuple(2.2, "world"s, 40),
                                std::make_tuple(2.2, "world"s, 40),  
                                std::make_tuple(1.1, "bili"s, 50)}, 
                                {"double_vec", "str_vec", "int_vec"});
    auto df3 = combine_inner<double>(df1, df2, "double_vec", 
                                    std::tuple<double, long>{}, {"double_vec", "long_vec"},
                                    std::tuple<double, std::string, int>{}, {"double_vec", "str_vec", "int_vec"});
    df3->print_with_index({0, 1, 2, 3});
    return 0;
}