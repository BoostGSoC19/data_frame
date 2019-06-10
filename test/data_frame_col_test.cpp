#include <boost/numeric/ublas/vector.hpp>
#include "data_frame_col.hpp"
#include <vector>
#include <iostream>
using namespace boost::numeric::ublas;
/* will be replaced by unit test frame work */
int main() {
    data_frame_col df;
    std::vector<int> tmp = {1, 2, 3, 4, 5};
    df.build_by_vec("int_vec", tmp);
    int len = df.get_size<int>();
    for (int i = 0; i < len; i++) {
        std::cout << df.at<int>(i) << std::endl;
    }
    data_frame_col df2("int_vec", tmp);
    for (int i = 0; i < len; i++) {
        std::cout << df2.at<int>(i) << std::endl;
    }
}