#ifndef _BOOST_UBLAS_DATA_FRAME_	
#define _BOOST_UBLAS_DATA_FRAME_
#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/storage.hpp>
#include "data_frame_col.hpp"
#include <list>
#include <string>
#include <unordered_map>
#include <vector>
#include <initializer_list>
#include <tuple>
#include <typeinfo>
#include <memory>
namespace boost { namespace numeric { namespace ublas {	
template<class... Types>
class data_frame;
template<typename... Types>
struct data_frame_type_builder {
    template<template<class...> class TypeLists, class... InnerTypes>
    constexpr data_frame_type_builder(TypeLists<InnerTypes...>){}
    using data_frame_type = data_frame<Types...>;
};
template<template<class...> class TypeLists, class... InnerTypes>
data_frame_type_builder(TypeLists<InnerTypes...>) -> data_frame_type_builder<InnerTypes...>;

template<class... Types>
class data_frame_view;

template<class... Types>
class data_frame {
public:
    using data_frame_type = data_frame<Types...>;
    using store_t = std::list<data_frame_col>;
    using name_map_t = std::unordered_map<std::string, typename std::list<data_frame_col>::iterator>;
    using type_map_t = std::unordered_map<std::string, std::string>;
    // InnerTypes must be unique
    template<template<class...> class TypeLists, class... InnerTypes>
    data_frame(TypeLists<InnerTypes...>): cur_rows(-1) { }
    template<template<class...> class TypeLists, class... InnerTypes>
    data_frame(int rows, TypeLists<InnerTypes...>): cur_rows(-1) { }
    template<typename T> 
    void add_column(std::string col_name, std::vector<T> tmp_vec) {
        static_assert(((std::is_same_v<T, Types> || ...)), "New column doesn't match any of the data_frame types!");
        assert(!col_names_map.count(col_name));
        if (cur_rows == -1) cur_rows = tmp_vec.size();
        assert(cur_rows == tmp_vec.size());
        data_frame_col dfc(col_name, tmp_vec);
        auto iter = vals.insert(vals.end(), dfc);
        col_names_map.insert({col_name, iter});
        type_map.insert({col_name, typeid(T).name()});
    }
    template<class... Args>
    void from_tuples(const std::vector<std::tuple<Args...>>& t, const std::vector<std::string>& names);
    template<class... Args>
    void from_tuples(std::initializer_list<std::tuple<Args...>> t, const std::vector<std::string>& names);
    template<template<class...> class TypeLists, class... Args>
    void from_tuples(const std::vector<std::tuple<Args...>>& t, const std::vector<std::string>& names, TypeLists<Args...>);
    template<typename T>
    T& get(const std::string& col_name, size_t pos);
    template<typename T>
    const T& get_c(const std::string& col_name, size_t pos) const;

//     template<typename Col_type, typename F, template<class...> class TypeLists, class... Types>
//     data_frame_view<TypeLists, Types...> select(const std::string& col_name, F f, TypeLists<Types...>) {
//         const std::vector<int>& new_order = filter<Col_type>(col_name, f);
//         return create_view_with_index(std::move(new_order), TypeLists<Types...>{});
//     }
//     template<typename Col_type, template<class...> class TypeLists, class... Types>
//     data_frame_view<TypeLists, Types...> sort(const std::string& col_name, TypeLists<Types...>) {
//         const std::vector<int>& new_order = order<Col_type>(col_name);
//         return create_view_with_index(std::move(new_order), TypeLists<Types...>{});
//     }
//     template<typename Col_type, typename F, template<class...> class TypeLists, class... Types>
//     data_frame_view<TypeLists, Types...> sort(const std::string& col_name, F f, TypeLists<Types...>) {
//         const std::vector<int>& new_order = order<Col_type>(col_name, f);
//         return create_view_with_index(std::move(new_order), TypeLists<Types...>{});
//     }
//     template<template<class...> class TypeLists, class... Types>
//     data_frame_view<TypeLists, Types...> create_view_with_index(std::vector<int>&& index, TypeLists<Types...>) {
//         return data_frame_view(this, std::move(index), TypeLists<Types...>{});
//     }
//     template<template<class...> class TypeLists, class... Types>
//     data_frame_view<TypeLists, Types...> create_view_with_index(const std::vector<int>& index, TypeLists<Types...>) {
//         return data_frame_view(this, index, TypeLists<Types...>{});
//     }
//     template<template<class...> class TypeLists>
//     data_frame_view<TypeLists, Types...> create_view_with_range(const range& r, TypeLists<Types...>) {
//         return data_frame_view(this, r, TypeLists<Types...>{});
//     }
//     template<template<class...> class TypeLists, class... Types>
//     data_frame_view<TypeLists, Types...> create_view_with_slice(const slice& s, TypeLists<Types...>) {
//         return data_frame_view(this, s, TypeLists<Types...>{});
//     }
    data_frame copy_with_index(const std::vector<int>& index) {
        int len = index.size();
        data_frame new_df;
        for (auto iter: col_names_map) {
            const auto& col_name = iter.first;
            initialize(col_name, len, [&new_df, col_name, len](auto& in) mutable {
                new_df.init_column<std::decay_t<decltype(in)>>(col_name, len);
            });
        }
        for (int i = 0; i < len; i++) {
            invoke_at(index[i], [&new_df, i](auto& in, const std::string& col_name) mutable {
                auto iter = new_df.col_names_map.find(col_name);
                if (iter == new_df.col_names_map.end()) return;
                auto& container = *(iter->second);
                container.data_frame_col::template at<std::decay_t<decltype(in)>>(i) = in;
            });
        }
        return new_df;
    }
    data_frame copy_with_range(const range& r) {
        int len = r.size();
        data_frame new_df;
        for (auto iter: col_names_map) {
            const auto& col_name = iter.first;
            initialize(col_name, len, [&new_df, col_name, len](auto& in) mutable {
                new_df.init_column<std::decay_t<decltype(in)>>(col_name, len);
            });
        }
        for (int i = 0; i < len; i++) {
            invoke_at(r(i), [&new_df, i](auto& in, const std::string& col_name) mutable {
                auto iter = new_df.col_names_map.find(col_name);
                if (iter == new_df.col_names_map.end()) return;
                auto& container = *(iter->second);
                container.data_frame_col::template at<std::decay_t<decltype(in)>>(i) = in;
            });
        }
        return new_df;
    }
    data_frame copy_with_slice(const slice& s) {
        int len = s.size();
        data_frame new_df;
        for (auto iter: col_names_map) {
            const auto& col_name = iter.first;
            initialize(col_name, len, [&new_df, col_name, len](auto& in) mutable {
                new_df.init_column<std::decay_t<decltype(in)>>(col_name, len);
            });
        }
        for (int i = 0; i < len; i++) {
            invoke_at(s(i), [&new_df, i](auto& in, const std::string& col_name) mutable {
                auto iter = new_df.col_names_map.find(col_name);
                if (iter == new_df.col_names_map.end()) return;
                auto& container = *(iter->second);
                container.data_frame_col::template at<std::decay_t<decltype(in)>>(i) = in;
            });
        }
        return new_df;
    }
//     template<typename F, template<class...> class TypeLists, class... Types>
//     void apply_with_index(const std::vector<int>& index, F f, TypeLists<Types...>) {
//         int len = index.size();
//         for (int i = 0; i < len; i++) {
//             apply_at<Types...>(index[i], [this, functor = f](auto& in){
//                 in = functor(in);
//             });
//         }
//     }
//     template<template<class...> class TypeLists, class... Types>
//     void print_with_index(const std::vector<int>& index, TypeLists<Types...>) {
//         int len = index.size();
//         for (int i = 0; i < len; i++) {
//             std::cout << "index " << index[i] <<": ";
//             apply_at<Types...>(index[i], [this](auto& in){
//                 std::cout << in << " ";
//             });
//             std::cout << '\n';
//         }
//     }
//     template<template<class...> class TypeLists, class... Types>
//     void print_with_range(const range& index, TypeLists<Types...>) {
//         int len = index.size();
//         for (int i = 0; i < len; i++) {
//             std::cout << "index " << index(i) <<": ";
//             apply_at<Types...>(index(i), [this](auto& in){
//                 std::cout << in << " ";
//             });
//             std::cout << '\n';
//         }
//     }
//     template<template<class...> class TypeLists, class... Types>
//     void print_with_slice(const slice& index, TypeLists<Types...>) {
//         int len = index.size();
//         for (int i = 0; i < len; i++) {
//             std::cout << "index " << index(i) <<": ";
//             apply_at<Types...>(index(i), [this](auto& in){
//                 std::cout << in << " ";
//             });
//             std::cout << '\n';
//         }
//     }
    int get_cur_rows() {
        return cur_rows;
    }
    int get_cur_cols() {
        return col_names_map.size();
    }
//     template<typename T>
//     std::vector<int> order(const std::string& col_name);
//     template<typename T, typename F>
//     std::vector<int> order(const std::string& col_name, F f);
// private:
//     template<typename T, typename F>
//     std::vector<int> filter(const std::string& col_name, F f);
    template<typename F>
    void invoke_at(int pos, F&& f) {
        for (auto iter: col_names_map) {
            const auto& col_name = iter.first;
            auto& container = *(iter.second);
            container.fill_data_at(pos, col_name, std::move(f), typename type_list<Types...>::types{});
        }
    }
//     template<typename... Types, typename F>
//     void apply_at(int pos, F f) {
//       for (auto iter: col_names_map) {
//             const auto& col_name = iter.first;
//             auto& container = *(iter.second);
//             container.apply_at(pos, std::move(f), typename type_list<Types...>::types{});
//         }
//     }
    template<typename F>
    void initialize(const std::string& col_name, int len, F f) {
        auto& container = col_names_map[col_name];
        container->initialize(std::move(f), typename type_list<Types...>::types{});
    }
    template<class... Args>
    void from_tuple(const std::tuple<Args...>& t, const std::vector<std::string>& names, int row) {
        for_each_in_tuple(t, [this, row](auto t, std::string name){
            auto iter = this->col_names_map.find(name);
            if (iter == this->col_names_map.end()) return;
            auto& container = *(iter->second);
            container.data_frame_col::template at<decltype(t)>(row) = t;   
        }, names);
    }
    template<typename T, typename F, std::size_t ... Is>
    void for_each(T&& t, F f, std::index_sequence<Is...>, const std::vector<std::string>& names) {
        auto l = { (f(std::get<Is>(t), names[Is]), 0)... };
    }
    template<typename... Ts, typename F>
    void for_each_in_tuple(std::tuple<Ts...> const& t, F f, const std::vector<std::string>& names) {
        for_each(t, f, std::index_sequence_for<Ts...>{}, names);
    }
    template<typename T>
    bool init_column(const std::string& col_name, int size);
    template<class... Args>
    void init_columns(const std::tuple<Args...>& t, const std::vector<std::string>& names, int size) {
        for_each_in_tuple(t, [this, size](auto t, const std::string& cur_name) {
            this->init_column<decltype(t)>(cur_name, size);
        }, names);
    }
    int cur_rows;
    store_t vals;
    /* col_names_map and type_map should maintain consistent */
    name_map_t col_names_map;
    type_map_t type_map;
};
// template deduction guide
template<template<class...> class TypeLists, class... InnerTypes>
data_frame(TypeLists<InnerTypes...>) -> data_frame<InnerTypes...>;
template<template<class...> class TypeLists, class... InnerTypes>
data_frame(int rows, TypeLists<InnerTypes...>) -> data_frame<InnerTypes...>;

template<class... Types>
template<typename T>
bool data_frame<Types...>::init_column(const std::string& col_name, int size) {
    if (col_names_map.count(col_name)) return false;
    /* size check */
    if (cur_rows == -1) cur_rows = size;
    if (cur_rows != size) return false;
    auto iter = vals.insert(vals.begin(), data_frame_col(col_name, std::vector<T>(size)));
    col_names_map.insert({col_name, iter});
    type_map.insert({col_name, typeid(T).name()});
    return true;
}
template<class... Types>
template<class... Args>
void data_frame<Types...>::from_tuples(std::initializer_list<std::tuple<Args...>> t, const std::vector<std::string>& names) {
    std::vector<std::tuple<Args...>> vec(t);
    from_tuples(vec, names);
}
template<class... Types>
template<class... Args>
void data_frame<Types...>::from_tuples(const std::vector<std::tuple<Args...>>& t, const std::vector<std::string>& names) {
    if (sizeof...(Args) != names.size()) return;
    cur_rows = t.size();
    init_columns(t[0], names, cur_rows);
    for (int i = 0; i < cur_rows; i++) {
        from_tuple(t[i], names, i);
    }
}
template<class... Types>
template<template<class...> class TypeLists, class... Args>
void data_frame<Types...>::from_tuples(const std::vector<std::tuple<Args...>>& t, const std::vector<std::string>& names, TypeLists<Args...>) {
    from_tuples<Args...>(t, names);
}
// template<typename T>
// std::vector<int> data_frame::order(const std::string& col_name) {
//     if (!col_names_map.count(col_name)) return {};
//     if (type_map[col_name] != typeid(T).name()) return {};
//     auto iter = col_names_map.find(col_name);
//     const auto& container = *(iter->second);
//     const auto& tmp_vector = container.get_vector<T>();
//     int len = tmp_vector.size();
//     std::vector<int> tmp_index;
//     for (int i = 0; i < tmp_vector.size(); i++) {
//         tmp_index.push_back(i);
//     }
//     auto cmp = [&](int& l, int& r) -> bool {
//         return tmp_vector[l] > tmp_vector[r];
//     };
//     std::sort(tmp_index.begin(), tmp_index.end(), cmp);
//     return tmp_index;
// }
// template<typename T, typename F>
// std::vector<int> data_frame::order(const std::string& col_name, F f) {
//     if (!col_names_map.count(col_name)) return {};
//     if (type_map[col_name] != typeid(T).name()) return {};
//     auto iter = col_names_map.find(col_name);
//     auto& container = *(iter->second);
//     auto& tmp_vector = container.get_vector<T>();
//     int len = tmp_vector.size();
//     std::vector<int> tmp_index;
//     for (int i = 0; i < tmp_vector.size(); i++) {
//         tmp_index.push_back(i);
//     }
//     auto cmp = [&](int& l, int& r) -> bool {
//         return f(tmp_vector[l], tmp_vector[r]);
//     };
//     std::sort(tmp_index.begin(), tmp_index.end(), cmp);
//     return tmp_index;
// }
// template<typename T, typename F>
// std::vector<int> data_frame::filter(const std::string& col_name, F f) {
//     if (!col_names_map.count(col_name)) return {};
//     if (type_map[col_name] != typeid(T).name()) return {};
//     auto iter = col_names_map.find(col_name);
//     auto& container = *(iter->second);
//     auto& tmp_vector = container.get_vector<T>();
//     int len = tmp_vector.size();
//     std::vector<int> tmp_index;
//     for (int i = 0; i < len; i++) {
//         if (f(tmp_vector[i]))
//             tmp_index.push_back(i);
//     }
//     return tmp_index;
// }
template<class... Types>
template<typename T>
T& data_frame<Types...>::get(const std::string& col_name, size_t pos) {
    // need to handle the case when col_name doesn't exist
    static_assert(((std::is_same_v<T, Types> || ...)), "Type doesn't match to data_frame");
    auto iter = col_names_map.find(col_name);
    auto& container = *(iter->second);
    auto& tmp_vector = container.data_frame_col::template get_vector<T>();
    return tmp_vector[pos];
}
template<class... Types>
template<typename T>
const T& data_frame<Types...>::get_c(const std::string& col_name, size_t pos) const {
    // need to handle the case when col_name doesn't exist
    static_assert(((std::is_same_v<T, Types> || ...)), "Type doesn't match to data_frame");
    auto iter = col_names_map.find(col_name);
    auto& container = *(iter->second);
    auto& tmp_vector = container.data_frame_col::template get_vector<T>();
    return tmp_vector[pos];
}
// A non-deduced context from tuple inside vector to make_from_tuples, have to provide additional parameter
template<template<class...> class TypeLists, class... InnerTypes>
auto make_from_tuples(const std::vector<TypeLists<InnerTypes...>>& t, const std::vector<std::string>& names) {
    using type_collection = typename type_list<InnerTypes...>::types;
    assert(sizeof...(InnerTypes) == names.size());
    int cur_rows = t.size();
    auto df = new data_frame(cur_rows, type_collection{});
    df->init_columns(t[0], names, cur_rows);
    for (int i = 0; i < cur_rows; i++)
        df->from_tuple(t[i], names, i);
    return df;
}
template<template<class...> class TypeLists, class... InnerTypes>
auto make_from_tuples(const std::vector<std::tuple<InnerTypes...>>& t, const std::vector<std::string>& names, 
    TypeLists<InnerTypes...>) {
    using type_collection = typename type_list<InnerTypes...>::types;
    assert(sizeof...(InnerTypes) == names.size());
    int cur_rows = t.size();
    auto df = new data_frame(cur_rows, type_collection{});
    df->init_columns(t[0], names, cur_rows);
    for (int i = 0; i < cur_rows; i++)
        df->from_tuple(t[i], names, i);
    return df;
}

template<class... Types>
class data_frame_view {
public:
    template<template<class...> class TypeLists, class... InnerTypes>
    data_frame_view(data_frame<InnerTypes...>* df, std::vector<int>&& index, TypeLists<InnerTypes...>): 
        data_frame_ptr(df), internal_index(std::move(index)) {}
    template<template<class...> class TypeLists, class... InnerTypes>
    data_frame_view(data_frame<InnerTypes...>* df, const std::vector<int>& index, TypeLists<InnerTypes...>) {
        data_frame_ptr = df;
        internal_index.clear();
        int len = index.size();
        for (int i = 0; i < len; i++)
            internal_index.push_back(index[i]);
    }
    template<template<class...> class TypeLists, class... InnerTypes>
    data_frame_view(data_frame<InnerTypes...>* df, const slice& index, TypeLists<InnerTypes...>) {
        data_frame_ptr = df;
        internal_index.clear();
        int len = index.size();
        for (int i = 0; i < len; i++)
            internal_index.push_back(index(i));
    }
    template<template<class...> class TypeLists, class... InnerTypes>
    data_frame_view(data_frame<InnerTypes...>* df, const range& index, TypeLists<InnerTypes...>) {
        data_frame_ptr = df;
        internal_index.clear();
        int len = index.size();
        for (int i = 0; i < len; i++)
            internal_index.push_back(index(i));
    }
//     template<typename F>
//     data_frame_view<TypeLists, Types...>& apply_with_index(const std::vector<int>& index, F f) {
//         data_frame_ptr->apply_with_index(index, f, TypeLists<Types...>{});
//         return *this;
//     }
//     void print_with_index(const std::vector<int>& index) {
//         data_frame_ptr->print_with_index(index, TypeLists<Types...>{});
//     }
//     void print_with_range(const range& index) {
//         data_frame_ptr->print_with_range(index, TypeLists<Types...>{});
//     }
//     void print_with_slice(const slice& index) {
//         data_frame_ptr->print_with_slice(index, TypeLists<Types...>{});
//     }
//     template<typename T>
//     data_frame_view<TypeLists, Types...>& sort(const std::string& col_name) {
//         data_frame_ptr->sort<T>(col_name, TypeLists<Types...>{});
//         return *this;
//     }
//     template<typename T, typename F>
//     data_frame_view<TypeLists, Types...>& sort(const std::string& col_name, F f) {
//         data_frame_ptr->sort<T>(col_name, f, TypeLists<Types...>{});
//         return *this;
//     }
//     template<typename T, typename F>
//     data_frame_view<TypeLists, Types...>& select(const std::string& col_name, F f) {
//         data_frame_ptr->select<T>(col_name, f, TypeLists<Types...>{});
//         return *this;
//     }
//     template<typename T>
//     T& get(const std::string& col_name, size_t pos) {
//         return data_frame_ptr->get<T>(col_name, pos);
//     }
//     template<typename T>
//     const T& get_c(const std::string& col_name, size_t pos) {
//         return data_frame_ptr->get_c<T>(col_name, pos);
//     }
//     size_t get_cur_rows() {
//         return internal_index.size();
//     }
//     size_t get_cur_cols() {
//         return data_frame_ptr->get_cur_cols();
//     }
private: 
    std::vector<int> internal_index;
    data_frame<Types...>* data_frame_ptr;
};
template<template<class...> class TypeLists, class... InnerTypes>
data_frame_view(data_frame<InnerTypes...>* df, std::vector<int>&& index, TypeLists<InnerTypes...>) -> data_frame_view<InnerTypes...>;
template<template<class...> class TypeLists, class... InnerTypes>
data_frame_view(data_frame<InnerTypes...>* df, const std::vector<int>& index, TypeLists<InnerTypes...>) -> data_frame_view<InnerTypes...>;
template<template<class...> class TypeLists, class... InnerTypes>
data_frame_view(data_frame<InnerTypes...>* df, const slice& index, TypeLists<InnerTypes...>) -> data_frame_view<InnerTypes...>;
template<template<class...> class TypeLists, class... InnerTypes>
data_frame_view(data_frame<InnerTypes...>* df, const range& index, TypeLists<InnerTypes...>) -> data_frame_view<InnerTypes...>;
}}}

#endif