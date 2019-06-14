#include <boost/numeric/ublas/vector.hpp>
#include "data_frame.hpp"
#include <vector>
#include <iostream>
#include <tuple>
#include "data_frame.hpp"
using namespace boost::numeric::ublas;
/* will be replaced by unit test frame work */
int main() {
    std::vector<int> int_vec = {1, 2, 3, 4, 5};
    std::vector<std::string> str_vec = {"test1", "test2", "test3", "test4", "test5"};
    data_frame df;
    df.add_column("int_vec", int_vec);
    df.add_column("str_vec", str_vec);
    std::tuple<int, double, std::string> t{1, 3.14, "hello"};
    data_frame df2;
    df2.from_tuples(std::vector{std::make_tuple(1, 3.3, "1"), 
                                std::make_tuple(2, 2.2, "2"), 
                                std::make_tuple(3, 1.1, "3")}, 
                    {"int_vec", "double_vec", "str_vec"});
    std::vector<int> new_index = df2.order<double>("double_vec");
    for (int i = 0; i < new_index.size(); i++) {
        std::cout << new_index[i] << std::endl;
    }
    auto df3 = df2.from_index<int, double, std::string>({1, 2, 0});
}