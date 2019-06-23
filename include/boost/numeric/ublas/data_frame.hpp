#ifndef _BOOST_UBLAS_DATA_FRAME_	
#define _BOOST_UBLAS_DATA_FRAME_
#include <boost/numeric/ublas/vector.hpp>
#include "data_frame_col.hpp"
#include <list>
#include <string>
#include <unordered_map>
#include <vector>
#include <tuple>
#include <typeinfo>
namespace boost { namespace numeric { namespace ublas {	
class data_frame {
public:
    using store_t = std::list<data_frame_col>;
    using name_map_t = std::unordered_map<std::string, typename std::list<data_frame_col>::iterator>;
    using type_map_t = std::unordered_map<std::string, std::string>;
    data_frame(): cur_rows(-1) {}
    data_frame(int rows): cur_rows(rows) {}
    template<typename T> 
    bool add_column(std::string col_name, std::vector<T> tmp_vec);
    template<class... Args>
    void from_tuples(const std::vector<std::tuple<Args...>>& t, const std::vector<std::string>& names);
    // return the new order
    template<typename T>
    std::vector<int> order(const std::string col_name);
    // use index to get a new data_frame
    template<typename... Types, typename F>
    void invoke_at(int pos, F&& f) {
        for (auto iter: col_names_map) {
            const auto& col_name = iter.first;
            auto& container = *(iter.second);
            container.fill_data_at(pos, col_name, std::move(f), typename type_list<Types...>::types{});
        }
    }
    template<typename... Types, typename F>
    void print_at(int pos, F f) {
      for (auto iter: col_names_map) {
            const auto& col_name = iter.first;
            auto& container = *(iter.second);
            container.print_at(pos, std::move(f), typename type_list<Types...>::types{});
        }
    }
    template<typename... Types, typename F>
    void initialize(const std::string& col_name, int len, F f) {
        auto& container = col_names_map[col_name];
        container->initialize(std::move(f), typename type_list<Types...>::types{});
    }
    template<template<class...> class TypeLists, class... Types>
    data_frame copy_with_index(const std::vector<int>& index, TypeLists<Types...>) {
        int len = index.size();
        data_frame new_df;
        for (auto iter: col_names_map) {
            const auto& col_name = iter.first;
            initialize<Types...>(col_name, len, [&new_df, col_name, len](auto& in) mutable {
                new_df.init_column<std::decay_t<decltype(in)>>(col_name, len);
            });
        }
        for (int i = 0; i < len; i++) {
            invoke_at<Types...>(index[i], [&new_df, i](auto& in, const std::string& col_name) mutable {
                auto iter = new_df.col_names_map.find(col_name);
                if (iter == new_df.col_names_map.end()) return;
                auto& container = *(iter->second);
                container.at<std::decay_t<decltype(in)>>(i) = in;
            });
        }
        return new_df;
    }
    template<template<class...> class TypeLists, class... Types>
    void print_with_index(const std::vector<int>& index, TypeLists<Types...>) {
        int len = index.size();
        for (int i = 0; i < len; i++) {
            std::cout << "index " << index[i] <<": ";
            print_at<Types...>(index[i], [this](auto& in){
                std::cout << in << " ";
            });
            std::cout << '\n';
        }
    }
    int get_cur_rows() {
        return cur_rows;
    }
    int get_cur_cols() {
        return col_names_map.size();
    }
private:
    template<class... Args>
    void from_tuple(const std::tuple<Args...>& t, const std::vector<std::string>& names, int row) {
        for_each_in_tuple(t, tuple_functor(this, row), names);
    }
    template<typename T, typename F, std::size_t ... Is>
    void for_each(T&& t, F f, std::index_sequence<Is...>, const std::vector<std::string>& names) {
        auto l = { (f(std::get<Is>(t), names[Is]), 0)... };
    }
    template<typename... Ts, typename F>
    void for_each_in_tuple(std::tuple<Ts...> const& t, F f, const std::vector<std::string>& names) {
        for_each(t, f, std::index_sequence_for<Ts...>{}, names);
    }
    struct tuple_functor {
        tuple_functor(data_frame* df, int index): df(df), index(index) {}
        template<typename T>
        void operator () (T t, std::string name) {
            auto iter = df->col_names_map.find(name);
            if (iter == df->col_names_map.end()) return;
            auto& container = *(iter->second);
            container.at<T>(index) = t;
        }
        int index;
        data_frame* df;
    };
    struct tuple_create_functor {
        tuple_create_functor(int size, data_frame* df): size(size) ,df(df){}
        template<typename T>
        void operator () (T t, const std::string& name) {
            df->init_column<T>(name, size);
        }
        int size;
        data_frame* df;
    };
    template<typename T>
    bool init_column(const std::string& col_name, int size);
    template<class... Args>
    void init_columns(const std::tuple<Args...>& t, const std::vector<std::string>& names, int size) {
        for_each_in_tuple(t, tuple_create_functor(size, this), names);
    }
    int cur_rows;
    store_t vals;
    /* col_names_map and type_map should maintain consistent */
    name_map_t col_names_map;
    type_map_t type_map;
};
template<typename T>
bool data_frame::add_column(std::string col_name, std::vector<T> tmp_vec) {
    if (col_names_map.count(col_name)) return false;
    /* size check */
    if (cur_rows == -1) cur_rows = tmp_vec.size();
    if (cur_rows != tmp_vec.size()) return false;
    data_frame_col dfc(col_name, tmp_vec);
    auto iter = vals.insert(vals.end(), dfc);
    col_names_map.insert({col_name, iter});
    type_map.insert({col_name, typeid(T).name()});
    return true;
}
template<typename T>
bool data_frame::init_column(const std::string& col_name, int size) {
    if (col_names_map.count(col_name)) return false;
    /* size check */
    if (cur_rows == -1) cur_rows = size;
    if (cur_rows != size) return false;
    auto iter = vals.insert(vals.begin(), data_frame_col(col_name, std::vector<T>(size)));
    col_names_map.insert({col_name, iter});
    type_map.insert({col_name, typeid(T).name()});
    return true;
}
template<class... Args>
void data_frame::from_tuples(const std::vector<std::tuple<Args...>>& t, const std::vector<std::string>& names) {
    if (sizeof...(Args) != names.size()) return;
    cur_rows = t.size();
    init_columns(t[0], names, cur_rows);
    for (int i = 0; i < cur_rows; i++) {
        from_tuple(t[i], names, i);
    }
    //print_index<Args...>({0, 1});
}
template<typename T>
std::vector<int> data_frame::order(const std::string col_name) {
    if (!col_names_map.count(col_name)) return {};
    if (type_map[col_name] != typeid(T).name()) return {};
    auto iter = col_names_map.find(col_name);
    auto& container = *(iter->second);
    auto& tmp_vector = container.get_vector<T>();
    int len = tmp_vector.size();
    std::vector<int> tmp_index;
    for (int i = 0; i < tmp_vector.size(); i++) {
        tmp_index.push_back(i);
    }
    auto cmp = [&](int& l, int& r) -> bool {
        return tmp_vector[l] > tmp_vector[r];
    };
    std::sort(tmp_index.begin(), tmp_index.end(), cmp);
    return tmp_index;
}
}}}

#endif