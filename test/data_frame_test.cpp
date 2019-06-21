#include <boost/numeric/ublas/vector.hpp>
#include "data_frame.hpp"
#include <vector>
#include <iostream>
#include <tuple>
#include <typeinfo>
#include <type_traits>
#include <string>
#include "data_frame.hpp"
using namespace boost::numeric::ublas;
using namespace std::string_literals;
/* will be replaced by unit test frame work */
int main() {
    std::vector<int> int_vec = {0, 1, 2, 3, 4};
    std::vector<std::string> str_vec = {"test0", "test1", "test2", "test3", "test4"};
    data_frame df;
    df.add_column("int_vec", int_vec);
    df.add_column("str_vec", str_vec);
    using type_collection = type_list<int, double, std::string>::types;    
    data_frame df2;
    df2.from_tuples(std::vector{std::make_tuple(1, 3.3, "hello"s), 
                                std::make_tuple(2, 2.2, "world"s), 
                                std::make_tuple(3, 1.1, "bili"s)}, 
                    {"int_vec", "double_vec", "str_vec"});
    std::vector<int> new_index = df2.order<double>("double_vec");
    df2.print_with_index({0, 1, 2}, type_collection{});
    auto df3 = df.copy_with_index({1, 2, 0, 2}, type_collection{});
    std::cout << "cur rows: " << df3.get_cur_rows() << std::endl;
    df3.print_with_index({0, 1, 2, 3}, type_collection{});
}