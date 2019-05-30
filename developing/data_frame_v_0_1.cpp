#include <any>
#include <map>
#include <set>
#include <vector>
#include <string>
#include <typeindex>
#include <typeinfo>
using namespace std;
class data_frame {
public:
    data_frame() = default;
    template<typename T>
    void add_cols(string name, vector<T>);
    vector<int> order(string name);  
private:
    int record;
    map<string, vector<any>> _stores;
    vector<int> index;
    set<string> colnames;
    map<string, string> type_map;
};

template<typename T>
void data_frame::add_cols(string name, vector<T> col) {
    type_map.insert({name, typeid(T).name()});
    if (record != 0 && col.size() != record) return;
    if (record == 0) {
        record = col.size();
        for (int i = 0; i < record; i++) {
            index.push_back(i);
        }
    }
    vector<any> tmp_col;
    for (int i = 0; i < col.size(); i++) {
        tmp_col.push_back(any(col[i]));
    }
    _stores.insert({name, tmp_col});
    colnames.insert(name);
}

vector<int> data_frame::order(string colname) {
    vector<int> new_index = index;
    if (colnames.count(colname)) {
        if (type_map[colname] == typeid(int).name()) {
            auto cmp = [&](int& l, int& r)->bool {
                return any_cast<int>(_stores[colname][l]) > any_cast<int>(_stores[colname][r]);
            };
            sort(new_index.begin(), new_index.end(), cmp);
        } else if (type_map[colname] == typeid(double).name()) {
            auto cmp = [&](int& l, int& r) -> bool {
                return any_cast<double>(_stores[colname][l]) > any_cast<double>(_stores[colname][r]);
            };
            sort(new_index.begin(), new_index.end(), cmp);
        } else if (type_map[colname] == typeid(string).name()) {
            auto cmp = [&](int& l, int& r) -> bool {
                return any_cast<string>(_stores[colname][l]) > any_cast<string>(_stores[colname][r]);
            };
            sort(new_index.begin(), new_index.end(), cmp);
        }
    }
    return new_index;
}

int main() {
   vector<string> strvec = {"b", "asdf", "hgdfh", "egfdsg", "fasgad"};
   vector<int> intvec = {1, 2, 3, 4,5};
   vector<double> doubvec = {1.2, 3.4, 1.5, 2.8, 7.2};
   data_frame df;
   df.add_cols<string>("strvec", strvec);
   df.add_cols<int>("intvec", intvec);
   df.add_cols<double>("doubvec", doubvec);
   vector<int> new_order = df.order("strvec");
}
