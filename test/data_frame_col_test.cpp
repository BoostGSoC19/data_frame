#include <boost/numeric/ublas/vector.hpp>
#include "data_frame_col.hpp"
#include "gtest/gtest.h"
#include <vector>
#include <iostream>
using namespace boost::numeric::ublas;
/* will be replaced by unit test frame work */

TEST(Data_frame_col, initialize) {
    data_frame_col df;
    std::vector<int> tmp = {1, 2, 3, 4, 5};
    df.build_by_vec("int_vec", tmp);
    int len = df.get_size<int>();
    EXPECT_EQ(5, len);
    for (int i = 0; i < len; i++) {
        EXPECT_EQ(i + 1, df.at<int>(i));
    }
}
