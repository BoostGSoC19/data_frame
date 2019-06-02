#include <boost/numeric/ublas/vector.hpp>
#include <map>
#include <string>
#include <memory>
#include <typeinfo>
#include <algorithm>
#include <iostream>
#include <tuple>
class col_base {

};
template<typename T> 
class col: public col_base {
public:
    using Type = T;
    using Container = boost::numeric::ublas::vector<Type>;
    col() = default;
    explicit col(std::string name, Container _store): col_name(name), _store(_store), curSize(_store.size()) {}
    explicit col(std::pair<std::string, Container> _p): col_name(_p.first), _store(_p.second), curSize(_p.second.size()) {}
    explicit col(const Container& other) : _store(other) {}
    void set_name(std::string name) { col_name = name; }
    std::string get_name() {return col_name; }
    int get_size() {return curSize; }
    void add_element(T val) {
        if (curSize >=  _store.size()) {
            _store.resize(curSize * 2);
        }
        _store.insert_element(curSize, val);
        curSize++;
    }
public:
    Container _store;
    std::string col_name;
    int curSize;
};
class data_frame {
public:
    data_frame() = default;
    template<typename T>
    void add_cols(std::string col_name, col<T>& col_vector) {
        if (_m.size() == 0) {
            index = boost::numeric::ublas::vector<int>(col_vector._store.size());
            for (int i = 0; i < col_vector._store.size(); i++) {
                index[i] = i;
            }
        }
        col_base* tmp = &col_vector;
        _type_map.insert({col_name, typeid(T).name()});
        _m[col_name] = tmp;
    }
    template<typename T> 
    std::vector<int> sort(std::string col_name) {
        if (!_type_map.count(col_name)) return {};
        if (typeid(T).name() != _type_map[col_name]) return {};
        std::vector<int> tmp_index;
        for (int i = 0; i < index.size(); i++) {
            tmp_index.push_back(index[i]);
        }
        col<T>* new_col = reinterpret_cast<col<T>*>(_m[col_name]);
        auto cmp = [&](int& l, int& r) -> bool {
            return new_col->_store[l] > new_col->_store[r];
        };
        std::sort(tmp_index.begin(), tmp_index.end(), cmp);
        return tmp_index;
    }
    /* The problem with this design approach is that it will make it hard to implement operations
    like join, since in template arguments, we can only specify the type of join on column, but cannot have 
    any information for other columns
    Another problem is when adding multiple rows, we need to provide information for each column. 
    */
    template<typename T>
    void add_val(T val, std::string col_name) {
        if (!_type_map.count(col_name)) return;
        if (typeid(T).name() != _type_map[col_name]) return;
        col<T>* new_col = reinterpret_cast<col<T>*>(_m[col_name]);
        /* Need to resize _store within each col */
        new_col->add_element(val);
    }
    template<int... Is>
    struct seq { };

    template<int N, int... Is>
    struct gen_seq : gen_seq<N - 1, N - 1, Is...> { };

    template<int... Is>
    struct gen_seq<0, Is...> : seq<Is...> { };
    template<typename T, typename F, int... Is>
    void for_each(T&& t, F f, seq<Is...>, std::vector<std::string>& names)
    {
        auto l = { (f(std::get<Is>(t), names[Is]), 0)... };
    }
    template<typename... Ts, typename F>
    void for_each_in_tuple(std::tuple<Ts...> const& t, F f, std::vector<std::string>& names)
    {
        for_each(t, f, gen_seq<sizeof...(Ts)>(), names);
    }
    struct tmp_functor
    {
        tmp_functor(data_frame* df): df(df) {}
        template<typename T>
        void operator () (T t, std::string val)
        {
            df->add_val<T>(t, val);
        }
        data_frame* df;
    };
    template<class... Args>
    void add_row(const std::tuple<Args...>& t, std::vector<std::string>& names) {
        for_each_in_tuple(t, tmp_functor(this), names);
    }
    template<class... Args>
    void add_rows(const std::vector<std::tuple<Args...>>& t, std::vector<std::string>& names) {
        for (int i = 0; i < t.size(); i++)
            add_row(t[i], names);
    }
    template<typename T> 
    void print_col(std::string col_name) {
        if (!_type_map.count(col_name)) return;
        if (typeid(T).name() != _type_map[col_name]) return ;
        col<T>* new_col = reinterpret_cast<col<T>*>(_m[col_name]);
        for (int i = 0; i < new_col->get_size(); i++) {
            std::cout << new_col->_store[i] << std::endl;
        }
    }
private:
    boost::numeric::ublas::vector<int> index;
    std::map<std::string, col_base*> _m;
    std::map<std::string, std::string> _type_map;
};

int main() {
    boost::numeric::ublas::vector<int> int_vec(10);
    for (int i = 0; i < 10; i++)
        int_vec[i] = i;
    col<int> int_cl("int_vec", int_vec);
    boost::numeric::ublas::vector<double> double_vec(10);
    for (int i = 0; i < 10; i++)
        double_vec[i] = i * 2.0;
    col<double> double_cl("double_vec", double_vec);  
    
    boost::numeric::ublas::vector<std::string> string_vec(10);
    for (int i = 0; i < 10; i++)
        string_vec[i] = std::to_string(i * 100);
    col<std::string> string_cl("string_vec", string_vec);  
    
    data_frame df;
    df.add_cols<int>(int_cl.get_name(), int_cl);
    df.add_cols<double>(double_cl.get_name(), double_cl);
    df.add_cols<std::string>(string_cl.get_name(), string_cl);
    std::vector<int> order = df.sort<double>("double_vec");
    for (int i = 0; i < order.size(); i++)
        std::cout << std::to_string(order[i]) << std::endl;
    std::cout << "-----" << std::endl;
    std::vector<int> order2 = df.sort<std::string>("string_vec");
    for (int i = 0; i < order2.size(); i++)
        std::cout << std::to_string(order2[i]) << std::endl;
    std::cout << "-----" << std::endl;   
    std::tuple<int, std::string, double> t1(10, "Test", 3.14);
    std::vector<std::string> tmp({"int_vec", "string_vec", "double_vec"});
    //df.print_row(t1, tmp);
    df.add_row(t1, tmp);
    df.print_col<std::string>("string_vec");
    std::cout << "-----" << std::endl;   
    std::vector<std::tuple<int, std::string, double>> t2{{10, "youtube", 3.14}, 
                                                          {2, "twitter", 6.0}, 
                                                          {4, "reddit", 7.24}};
    df.add_rows(t2, tmp);
    df.print_col<std::string>("string_vec");
    std::cout << "-----" << std::endl;   
    df.print_col<int>("int_vec");
}