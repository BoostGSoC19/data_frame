#include <boost/any.hpp>
#include <boost/numeric/ublas/vector.hpp>
#include <string>
#include <set>
#include <unordered_map>
#include <map>
#include <vector>
#include <iostream>
using Index = std::set<int>;
using namespace std;
template<typename T>
class data_frame;

template<typename T, typename Label> 
class series {
    //using namespace boost::numeric::ublas;
public:
    friend class data_frame<T>;
    series(vector<T>& vals, vector<Label>& labels): vals(vals), indexs(labels) { }
    vector<T> vals;
    vector<Label> indexs;

};

template<typename T>
class data_frame {
    //using namespace boost::numeric::ublas;
public:
    data_frame() = default;
    void add_cols(string name, series<boost::any, T>& data) {
        stores.insert({name, data});
        colnames.insert(name);
        if (labels.size() != data.indexs.size()) {
            labels = data.indexs;
            int i = 0;
            for (auto iter = labels.begin(); iter != labels.end(); iter++) {
                labelsMap[*iter] = ++i;
            }
        }
    }
    void sort(string name) {
        if (!colnames.count(name)) {
            cout << "name this col" << endl;
            return;
        }
        // need to cast type back and sort, may require a type map
        // fetch type infomation from any time again and sort on that
    }
    std::unordered_map<std::string, series<boost::any, T>> stores; 
    std::map<T, int> labelsMap;
    vector<T> labels;
    std::set<std::string> colnames;
};

int main() {
    vector<int> labels = {1, 2, 3, 4, 5, 6};
    vector<boost::any> stringvals = {"asdfj", "asdlfj", "asdtrew", "jkjhf", "fdhfdsg", "adslkfjio"};
    vector<boost::any> floatvals = {1.23, 4.32, 2.38, 7.21, 3.14, 6.28};
    vector<boost::any> intvals = {1, 2, 3, 4, 5, 6};
    series<boost::any, int> s1(stringvals, labels);
    series<boost::any, int> s2(floatvals, labels);
    series<boost::any, int> s3(intvals, labels);
    data_frame<int> df;
    df.add_cols("string", s1);
    df.add_cols("float", s2);
    df.add_cols("int", s3);

}
