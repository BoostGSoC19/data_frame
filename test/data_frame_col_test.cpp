#include <boost/numeric/ublas/vector.hpp>
#include "../include/boost/numeric/ublas/data_frame_col.hpp"
#include <vector>
#include <iostream>
using namespace boost::numeric::ublas;
int main() {
    data_frame_col df;
    std::vector<int> tmp = {1, 2, 3, 4, 5};
    df.add_column(tmp);
    int len = df.getSize<int>();
    for (int i = 0; i < len; i++) {
        std::cout << df.at<int>(i) << std::endl;
    }
}