#define BOOST_TEST_MODULE TEST_DATA_FRAME_COL
#define BOOST_TEST_DYN_LINK
#include <boost/numeric/ublas/vector.hpp>
#include <boost/test/unit_test.hpp>
#include "data_frame_col.hpp"
#include <vector>
#include <iostream>
using namespace boost::numeric::ublas;
/* will be replaced by unit test frame work */
BOOST_AUTO_TEST_SUITE(test_data_frame_col)

BOOST_AUTO_TEST_CASE(data_frame_col_initialize) {
    data_frame_col df;
    std::vector<int> tmp = {1, 2, 3, 4, 5};
    df.build_by_vec("int_vec", tmp);
    int len = df.get_size<int>();
    BOOST_CHECK_EQUAL(5, len);
    for (int i = 0; i < len; i++) {
        BOOST_CHECK_EQUAL(i + 1, df.at<int>(i));
    }
}
BOOST_AUTO_TEST_SUITE_END()