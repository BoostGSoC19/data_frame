#ifndef _BOOST_UBLAS_DATA_FRAME_	
#define _BOOST_UBLAS_DATA_FRAME_
#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/storage.hpp>
#include "data_frame_col.hpp"
#include <list>
#include <string>
#include <unordered_map>
#include <map>
#include <set>
#include <vector>
#include <initializer_list>
#include <tuple>
#include <typeinfo>
#include <type_traits>
#include <memory>
namespace boost { namespace numeric { namespace ublas {	
template<class... Types>
class data_frame;
/** @brief @code data_frame_type_builder @endcode is used to provide a more convenient type declaration 
 * @code 
 * using type_collection = type_list<int, double, std::string>::original_types; 
 * data_frame_type_builder type_builder = {type_collection{}};
 * using data_frame_type = decltype(type_builder)::data_frame_type;
 * data_frame_type* df3 = make_from_tuples({ {1, 3.3, "hello"s}, 
 *                                           {2, 2.2, "world"s}, 
 *                                           {3, 1.1, "github"s}   }, 
 *                                           {"int_vec", "double_vec", "str_vec"}, 
 *                                           type_collection{});
 * @endcode
 * 
 * @tparam TypeLists a set of potential types, used as @code TypeLists<Types...> @endcode
 * 
 * @tparam Types... a typelists containing concrete types in @code TypeLists<Types...> @endcode
 */
template<typename... Types>
struct data_frame_type_builder {
    template<template<class...> class TypeLists, class... InnerTypes>
    constexpr data_frame_type_builder(TypeLists<InnerTypes...>){}
    using data_frame_type = data_frame<Types...>;
};
template<template<class...> class TypeLists, class... InnerTypes>
data_frame_type_builder(TypeLists<InnerTypes...>) -> data_frame_type_builder<InnerTypes...>;
template<typename... Ts, typename... Types, std::size_t ... Is>
auto for_each(const std::tuple<Ts...>& t, const data_frame<Types...>* df, 
                const std::vector<std::string>& names, size_t pos, std::index_sequence<Is...>) {
    return std::make_tuple(df->data_frame<Types...>::template get_c<Ts>(names[Is], pos)...);
}
/** @brief used to iterate each value in @code std::tuple @endcode 
 * 
 * @tparam Ts...  types
 * 
 * @tparam Types... a typelists containing concrete types in @code TypeLists<Types...> @endcode
 * 
 * @param t one tuple used for deduct types
 * 
 * @param df the @code data_frame @endcode operating on
 * 
 * @param names the corresponding column name for each type in Types...
 * 
 * @param pos the row index for df
 */
template<typename... Types, typename... Ts>
auto for_each_in_tuple(const std::tuple<Ts...>& t, const data_frame<Types...>* df, 
                        const std::vector<std::string>& names, size_t pos) {
    return for_each(t, df, names, pos, std::index_sequence_for<Ts...>{});
}
/** @brief create a new @code data_frame @endcode from a vector of tuples
 * 
 * @tparam TypeLists... a container for types
 * 
 * @tparam InnerTypes... a typelists containing concrete types in @code TypeLists<Types...> @endcode
 * 
 * @param t a vector of tuples with InnerTypes...
 * 
 * @param names the corresponding column name for each type in InnerTypes...
 * 
 * @param TypeLists<InnerTypes...> used for type deduction
 */
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
class data_frame_view;

/** @brief data_frame represents a collection of data_frame_col, and it's designed as a heterogenous container. 
 * each data_frame_col can represent only one type 
 * 
 * @tparam Types... represent a non-repeated types from all data_frame_col
 */
template<class... Types>
class data_frame {
public:
    using data_frame_type = data_frame<Types...>;
    using store_t = std::list<data_frame_col>;
    using name_map_t = std::unordered_map<std::string, typename std::list<data_frame_col>::iterator>;
    using type_map_t = std::unordered_map<std::string, std::string>;
    /** @brief Build an empty data_frame
     *
     * @note current column is empty
     */
    data_frame(): cur_rows(-1) { }
    /** @brief Build an empty data_frame
    *
    * @tparam TypeLists a set of potential types, used as @code TypeLists<InnerTypes...> @endcode
    * 
    * @tparam InnerTypes... a typelists containing concrete types in @code TypeLists<InnerTypes...> @endcode
    */
    template<template<class...> class TypeLists, class... InnerTypes>
    data_frame(TypeLists<InnerTypes...>): cur_rows(-1) { }
    /** @brief Build an empty data_frame
    *
    * @tparam TypeLists a set of potential types, used as @code TypeLists<InnerTypes...> @endcode
    * 
    * @tparam InnerTypes... a typelists containing concrete types in @code TypeLists<InnerTypes...> @endcode
    */
    template<template<class...> class TypeLists, class... InnerTypes>
    data_frame(size_t rows, TypeLists<InnerTypes...>): cur_rows(rows) { }
    /** @brief Add one column with col_name as name, tmp_vec as column data
    *
    * @tparam T T must be one of the Types... 
    * 
    * @param col_name column name for new column
    * 
    * @param tmp_vec data for new column
    */   
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
    /** @brief add tuples with colname @code names @endcode
    *
    * @tparam InnerTypes... The type for tuple, each InnerTypes must be one of Types... 
    * 
    * @param t vector of tuples, the size will become data_frame row size
    * 
    * @param names column name for each tuple type, maintains a mapping for tuple type to column name
    */       
    template<class... InnerTypes>
    void from_tuples(const std::vector<std::tuple<InnerTypes...>>& t, const std::vector<std::string>& names);
    /** @brief add tuples with colname @code names @endcode
    *
    * @tparam InnerTypes... The type for tuple, each InnerTypes must be one of Types... 
    * 
    * @param t initializer_list of tuples, the size will become data_frame row size
    * 
    * @param names column name for each tuple type, maintains a mapping for tuple type to column name
    */      
    template<class... InnerTypes>
    void from_tuples(std::initializer_list<std::tuple<InnerTypes...>> t, const std::vector<std::string>& names);
    /** @brief add tuples with colname @code names @endcode
    *
    * @tparam TypeLists container for InnerTypes... 
    * 
    * @tparam InnerTypes... The type for tuple, each InnerTypes must be one of Types... 
    * 
    * @param t vector of tuples, the size will become data_frame row size
    * 
    * @param names column name for each tuple type, maintains a mapping for tuple type to column name
    * 
    * @param TypeLists<Args...> used for type deduction
    */   
    template<template<class...> class TypeLists, class... InnerTypes>
    void from_tuples(const std::vector<std::tuple<InnerTypes...>>& t, const std::vector<std::string>& names, TypeLists<InnerTypes...>);
    /** @brief return data with at pos row in col_name position
    *
    * @tparam T the type for col_name column 
    *  
    * @param pos row position
    */   
    template<typename T>
    T& get(const std::string& col_name, size_t pos);
    /** @brief return const data with at pos row in col_name position
    *
    * @tparam T the type for col_name column 
    *  
    * @param pos row position
    */   
    template<typename T>
    const T& get_c(const std::string& col_name, size_t pos) const;
    /** @brief create a view only contains first n lines
    * 
    * @param n first n lines
    */   
    data_frame_view<Types...> head(size_t n) {
        assert(n < cur_rows);
        return create_view_with_range(range(0, n));
    }
   /** @brief create a view only contains last n lines
    * 
    * @param n last n lines
    */   
    data_frame_view<Types...> tail(size_t n) {
        assert(n < cur_rows);
        return create_view_with_range(range(cur_rows - n, cur_rows));
    }
    /** @brief create a view with rows only satisfying a condition 
    * 
    * @tparam Col_type the column type to be filtered
    * 
    * @tparam F a functor for condition
    * 
    * @param col_name the column name applying the condition
    * 
    * @param f a functor for condition
    */   
    template<typename Col_type, typename F>
    data_frame_view<Types...> select(const std::string& col_name, F f) {
        const std::vector<int>& new_order = filter<Col_type>(col_name, f);
        return create_view_with_index(std::move(new_order));
    }
    /** @brief create a view with new row orders after sort
    * 
    * @tparam Col_type the column type to be filtered
    * 
    * @param col_name the column name applying the condition
    */   
    template<typename Col_type>
    data_frame_view<Types...> sort(const std::string& col_name) {
        const std::vector<int>& new_order = order<Col_type>(col_name);
        return create_view_with_index(std::move(new_order));
    }
    /** @brief create a view with new row orders after sort
    * 
    * @tparam Col_type the column type to be filtered
    * 
    * @tparam F a functor for condition
    * 
    * @param col_name the column name applying the condition
    * 
    * @param f a functor for condition
    */   
    template<typename Col_type, typename F>
    data_frame_view<Types...> sort(const std::string& col_name, F f) {
        const std::vector<int>& new_order = order<Col_type>(col_name, f);
        return create_view_with_index(std::move(new_order));
    }
    /** @brief create a view with current data_frame
    * 
    * @param index the index number to create data_frame_view
    */   
    data_frame_view<Types...> create_view_with_index(std::vector<int>&& index) {
        return data_frame_view(this, std::move(index), typename type_list<Types...>::types{});
    }
    /** @brief create a view with current data_frame
    * 
    * @param index the index number to create data_frame_view
    */   
    data_frame_view<Types...> create_view_with_index(const std::vector<int>& index) {
        return data_frame_view(this, index, typename type_list<Types...>::types{});
    }
    /** @brief create a view with current data_frame
    * 
    * @param r the index range to create data_frame_view
    */   
    data_frame_view<Types...> create_view_with_range(const range& r) {
        return data_frame_view(this, r, typename type_list<Types...>::types{});
    }
    /** @brief create a view with current data_frame
    * 
    * @param s the index slice to create data_frame_view
    */   
    data_frame_view<Types...> create_view_with_slice(const slice& s) {
        return data_frame_view(this, s, typename type_list<Types...>::types{});
    }
   /** @brief copy a new data_frame with existing data
    * 
    * @param index the index copy from current data_frame
    */   
    data_frame<Types...> copy_with_index(const std::vector<int>& index) {
        int len = index.size();
        data_frame<Types...> new_df;
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
    /** @brief copy a new data_frame with existing data
    * 
    * @param index the range copy from current data_frame
    */   
    data_frame<Types...> copy_with_range(const range& r) {
        int len = r.size();
        data_frame<Types...> new_df;
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
    /** @brief copy a new data_frame with existing data
    * 
    * @param index the slice copy from current data_frame
    */   
    data_frame<Types...> copy_with_slice(const slice& s) {
        int len = s.size();
        data_frame<Types...> new_df;
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
    /** @brief change existing value for specific indexes
    * 
    * @tparam F the functor type for applying
    * 
    * @param index the index for using the applying the computation
    * 
    * @param f the functor to compute new value
    */   
    template<typename F>
    void apply_with_index(const std::vector<int>& index, F f) {
        int len = index.size();
        for (int i = 0; i < len; i++) {
            apply_at(index[i], [this, functor = f](auto& in){
                in = functor(in);
            });
        }
    }
    /** @brief print for specific indexes
    * 
    * @param index the index for printing
    */   
    void print_with_index(const std::vector<int>& index) {
        int len = index.size();
        for (int i = 0; i < len; i++) {
            std::cout << "index " << index[i] <<": ";
            apply_at(index[i], [this](auto& in){
                std::cout << in << " ";
            });
            std::cout << '\n';
        }
        std::cout << std::endl;
    }
    /** @brief print for specific indexes
    * 
    * @param index the range for printing
    */   
    void print_with_range(const range& index) {
        int len = index.size();
        for (int i = 0; i < len; i++) {
            std::cout << "index " << index(i) <<": ";
            apply_at(index(i), [this](auto& in){
                std::cout << in << " ";
            });
            std::cout << '\n';
        }
    }
    /** @brief print for specific indexes
    * 
    * @param index the slice for printing
    */   
    void print_with_slice(const slice& index) {
        int len = index.size();
        for (int i = 0; i < len; i++) {
            std::cout << "index " << index(i) <<": ";
            apply_at(index(i), [this](auto& in){
                std::cout << in << " ";
            });
            std::cout << '\n';
        }
    }
    /** @brief inner join two data frames on specific column 
    * 
    * @tparam T the type of the column to be joined
    * 
    * @tparam Types2... the template argument for another data_frame
    * 
    * @tparam TypeLists1 the type container for current data_frame 
    * 
    * @tparam InnerTypes1... the types for current data_frame
    * 
    * @tparam TypeLists2 the type conatiner for the second data_frame
    * 
    * @tparam InnerTypes2... the types for the second data_frame
    * 
    * @param other another data_frame to be joined
    * 
    * @param col_name the name of column to be joined on
    * 
    * @param TypeLists1<InnerTypes1...> used to deduct tuple type for current data_frame
    * 
    * @param colnamesl the corresponding column names for each type position in TypeLists1<InnerTypes1...>
    * 
    * @param TypeLists2<InnerTypes2...> used to deduct tuple type for second data_frame
    * 
    * @param colnamesr the corresponding column names for each type position in TypeLists2<InnerTypes2...>
    */      
    template<typename T,
                        typename... Types2, 
                        template<class...> class TypeLists1, typename... InnerTypes1, 
                        template<class...> class TypeLists2, typename... InnerTypes2>
    auto combine_inner(const data_frame<Types2...>& other, 
        const std::string& col_name, 
        TypeLists1<InnerTypes1...>, const std::vector<std::string>& colnamesl, 
        TypeLists2<InnerTypes2...>, const std::vector<std::string>& colnamesr);
    /** @brief left join two data frames on specific column 
    * 
    * @tparam T the type of the column to be joined
    * 
    * @tparam Types2... the template argument for another data_frame
    * 
    * @tparam TypeLists1 the type container for current data_frame 
    * 
    * @tparam InnerTypes1... the types for current data_frame
    * 
    * @tparam TypeLists2 the type conatiner for the second data_frame
    * 
    * @tparam InnerTypes2... the types for the second data_frame
    * 
    * @param other another data_frame to be joined
    * 
    * @param col_name the name of column to be joined on
    * 
    * @param TypeLists1<InnerTypes1...> used to deduct tuple type for current data_frame
    * 
    * @param colnamesl the corresponding column names for each type position in TypeLists1<InnerTypes1...>
    * 
    * @param TypeLists2<InnerTypes2...> used to deduct tuple type for second data_frame
    * 
    * @param colnamesr the corresponding column names for each type position in TypeLists2<InnerTypes2...>
    */   
    template<typename T,
                        typename... Types2, 
                        template<class...> class TypeLists1, typename... InnerTypes1, 
                        template<class...> class TypeLists2, typename... InnerTypes2>
    auto combine_left(const data_frame<Types2...>& other, 
        const std::string& col_name, 
        TypeLists1<InnerTypes1...>, const std::vector<std::string>& colnamesl, 
        TypeLists2<InnerTypes2...>, const std::vector<std::string>& colnamesr);
    /** @brief right join two data frames on specific column 
    * 
    * @tparam T the type of the column to be joined
    * 
    * @tparam Types2... the template argument for another data_frame
    * 
    * @tparam TypeLists1 the type container for current data_frame 
    * 
    * @tparam InnerTypes1... the types for current data_frame
    * 
    * @tparam TypeLists2 the type conatiner for the second data_frame
    * 
    * @tparam InnerTypes2... the types for the second data_frame
    * 
    * @param other another data_frame to be joined
    * 
    * @param col_name the name of column to be joined on
    * 
    * @param TypeLists1<InnerTypes1...> used to deduct tuple type for current data_frame
    * 
    * @param colnamesl the corresponding column names for each type position in TypeLists1<InnerTypes1...>
    * 
    * @param TypeLists2<InnerTypes2...> used to deduct tuple type for second data_frame
    * 
    * @param colnamesr the corresponding column names for each type position in TypeLists2<InnerTypes2...>
    */   
    template<typename T,
                        typename... Types2, 
                        template<class...> class TypeLists1, typename... InnerTypes1, 
                        template<class...> class TypeLists2, typename... InnerTypes2>
    auto combine_right(const data_frame<Types2...>& other, 
        const std::string& col_name, 
        TypeLists1<InnerTypes1...>, const std::vector<std::string>& colnamesl, 
        TypeLists2<InnerTypes2...>, const std::vector<std::string>& colnamesr);
    /** @brief full join two data frames on specific column 
    * 
    * @tparam T the type of the column to be joined
    * 
    * @tparam Types2... the template argument for another data_frame
    * 
    * @tparam TypeLists1 the type container for current data_frame 
    * 
    * @tparam InnerTypes1... the types for current data_frame
    * 
    * @tparam TypeLists2 the type conatiner for the second data_frame
    * 
    * @tparam InnerTypes2... the types for the second data_frame
    * 
    * @param other another data_frame to be joined
    * 
    * @param col_name the name of column to be joined on
    * 
    * @param TypeLists1<InnerTypes1...> used to deduct tuple type for current data_frame
    * 
    * @param colnamesl the corresponding column names for each type position in TypeLists1<InnerTypes1...>
    * 
    * @param TypeLists2<InnerTypes2...> used to deduct tuple type for second data_frame
    * 
    * @param colnamesr the corresponding column names for each type position in TypeLists2<InnerTypes2...>
    */   
    template<typename T,
                        typename... Types2, 
                        template<class...> class TypeLists1, typename... InnerTypes1, 
                        template<class...> class TypeLists2, typename... InnerTypes2>
    auto combine_full(const data_frame<Types2...>& other, 
        const std::string& col_name, 
        TypeLists1<InnerTypes1...>, const std::vector<std::string>& colnamesl, 
        TypeLists2<InnerTypes2...>, const std::vector<std::string>& colnamesr);
    /** @brief get current row number 
    */     
    int get_cur_rows() const {
        return cur_rows;
    }
    /** @brief get current column number 
    */  
    int get_cur_cols() const {
        return col_names_map.size();
    }
    /** @brief get current column names 
    */  
    std::vector<std::string> get_col_names() const {
        std::vector<std::string> ans;
        for (const auto& p: type_map)
            ans.push_back(p.first);
        return ans;
    }
    /** @brief initialize values for @code data_frame @endcode
    *  
    * @tparam InnerTypes... a typelists containing concrete types
    * 
    * @param t vector of tuples
    * 
    * @param names the corresponding names for each tuple component
    * 
    * @param size row number
    */
    template<class... InnerTypes>
    void init_columns(const std::tuple<InnerTypes...>& t, const std::vector<std::string>& names, int size) {
        for_each_in_tuple(t, [this, size](auto t, const std::string& cur_name) {
            this->init_column<decltype(t)>(cur_name, size);
        }, names);
    }
    /** @brief initialize specific column for @code data_frame @endcode
    *  
    * @tparam T a type for current column
    * 
    * @param col_name the column name to be initialized
    * 
    * @param size row number
    */
    template<typename T>
    bool init_column(const std::string& col_name, int size);
    template<class... InnerTypes>
    void from_tuple(const std::tuple<InnerTypes...>& t, const std::vector<std::string>& names, int size) {
        for_each_in_tuple(t, [this, size](auto t, std::string name){
            auto iter = this->col_names_map.find(name);
            if (iter == this->col_names_map.end()) return;
            auto& container = *(iter->second);
            container.data_frame_col::template at<decltype(t)>(size) = t;   
        }, names);
    }
    /** @brief delete specific column 
    *  
    * @tparam T a type for current column
    * 
    * @param col_name the column name to be deleted
    */
    template<typename T>
    void remove_col(const std::string& col_name) {
        type_map.erase(col_name);
        auto iter = col_names_map.find(col_name);
        if (iter == col_names_map.end()) return;
        vals.erase(iter->second);
        col_names_map.erase(col_name);
    }
    /** @brief return a new index order after sorting for column with name col_name
    *  
    * @tparam T a type for current column
    * 
    * @param col_name the column name to be sorted
    */
    template<typename T>
    std::vector<int> order(const std::string& col_name);
    /** @brief return a new index order after sorting for column with name col_name
    *  
    * @tparam F a user defined functor type
    * 
    * @tparam T a type for current column
    * 
    * @param col_name the column name to be sorted
    * 
    * @param f a user defined funtor to sort on specific column
    */
    template<typename T, typename F>
    std::vector<int> order(const std::string& col_name, F f);
private:
    template<typename T, typename F>
    std::vector<int> filter(const std::string& col_name, F f);
    template<typename F>
    void invoke_at(int pos, F&& f) {
        for (auto iter: col_names_map) {
            const auto& col_name = iter.first;
            auto& container = *(iter.second);
            container.fill_data_at(pos, col_name, std::move(f), typename type_list<Types...>::types{});
        }
    }
    template<typename F>
    void apply_at(int pos, F f) {
      for (auto iter: col_names_map) {
            const auto& col_name = iter.first;
            auto& container = *(iter.second);
            container.apply_at(pos, std::move(f), typename type_list<Types...>::types{});
        }
    }
    template<typename F>
    void initialize(const std::string& col_name, int len, F f) {
        auto& container = col_names_map[col_name];
        container->initialize(std::move(f), typename type_list<Types...>::types{});
    }
    template<typename T, typename F, std::size_t ... Is>
    void for_each(T&& t, F f, std::index_sequence<Is...>, const std::vector<std::string>& names) {
        auto l = { (f(std::get<Is>(t), names[Is]), 0)... };
    }
    template<typename... Ts, typename F>
    void for_each_in_tuple(std::tuple<Ts...> const& t, F f, const std::vector<std::string>& names) {
        for_each(t, f, std::index_sequence_for<Ts...>{}, names);
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
template<class... Types>
template<typename T>
std::vector<int> data_frame<Types...>::order(const std::string& col_name) {
    static_assert(((std::is_same_v<T, Types> || ...)), "Type doesn't match to data_frame");
    if (!col_names_map.count(col_name)) return {};
    if (type_map[col_name] != typeid(T).name()) return {};
    auto iter = col_names_map.find(col_name);
    const auto& container = *(iter->second);
    const auto& tmp_vector = container.data_frame_col::template get_vector<T>();
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
template<class... Types>
template<typename T, typename F>
std::vector<int> data_frame<Types...>::order(const std::string& col_name, F f) {
    static_assert(((std::is_same_v<T, Types> || ...)), "Type doesn't match to data_frame");
    if (!col_names_map.count(col_name)) return {};
    if (type_map[col_name] != typeid(T).name()) return {};
    auto iter = col_names_map.find(col_name);
    auto& container = *(iter->second);
    auto& tmp_vector = container.data_frame_col::template get_vector<T>();
    int len = tmp_vector.size();
    std::vector<int> tmp_index;
    for (int i = 0; i < tmp_vector.size(); i++) {
        tmp_index.push_back(i);
    }
    auto cmp = [&](int& l, int& r) -> bool {
        return f(tmp_vector[l], tmp_vector[r]);
    };
    std::sort(tmp_index.begin(), tmp_index.end(), cmp);
    return tmp_index;
}
template<class... Types>
template<typename T, typename F>
std::vector<int> data_frame<Types...>::filter(const std::string& col_name, F f) {
    static_assert(((std::is_same_v<T, Types> || ...)), "Type doesn't match to data_frame");
    if (!col_names_map.count(col_name)) return {};
    if (type_map[col_name] != typeid(T).name()) return {};
    auto iter = col_names_map.find(col_name);
    auto& container = *(iter->second);
    auto& tmp_vector = container.data_frame_col::template get_vector<T>();
    int len = tmp_vector.size();
    std::vector<int> tmp_index;
    for (int i = 0; i < len; i++) {
        if (f(tmp_vector[i]))
            tmp_index.push_back(i);
    }
    return tmp_index;
}
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
    static_assert(((std::is_same_v<T, Types> || ...)), "Type doesn't match to data_frame");
    auto iter = col_names_map.find(col_name);
    auto& container = *(iter->second);
    auto& tmp_vector = container.data_frame_col::template get_vector<T>();
    return tmp_vector[pos];
}
template<class... Types>
template<typename T,
                    typename... Types2, 
                    template<class...> class TypeLists1, typename... InnerTypes1, 
                    template<class...> class TypeLists2, typename... InnerTypes2>
auto data_frame<Types...>::combine_inner(const data_frame<Types2...>& other, 
    const std::string& col_name, 
    TypeLists1<InnerTypes1...>, const std::vector<std::string>& colnamesl, 
    TypeLists2<InnerTypes2...>, const std::vector<std::string>& colnamesr) {
    static_assert(((std::is_same_v<T, Types> || ...)), "T type doesn't belong to common types");
    static_assert(((std::is_same_v<T, Types2> || ...)), "T type doesn't belong to common types");
    assert(sizeof...(InnerTypes1) == colnamesl.size());
    assert(sizeof...(InnerTypes2) == colnamesr.size());
    using type_collection_l = typename type_list<Types...>::types;
    using type_collection_r = typename type_list<Types2...>::types;
    auto merge_type_collection = merge_types(type_collection_l{}, type_collection_r{});
    int llen = get_cur_rows();
    int rlen = other.get_cur_rows();
    std::multimap<T, size_t> valueTopos1;
    std::multimap<T, size_t> valueTopos2;
    // Must iterate twice to get the number of rows in result data frame
    for (int i = 0; i < llen; i++)
        valueTopos1.insert({get_c<T>(col_name, i), i});
    for (int j = 0; j < rlen; j++) {
        const T& val = other.data_frame<Types2...>::template get_c<T>(col_name, j);
        if (valueTopos1.count(val))
            valueTopos2.insert({val, j});
    }
    // get concated tuple type and names
    auto tuple_cat_val = std::tuple_cat(std::tuple<InnerTypes1...>{}, std::tuple<InnerTypes2...>{});
    std::vector<std::string> col_names;
    for (const auto& l_name: colnamesl) { col_names.push_back(l_name); }
    for (const auto& r_name: colnamesr) { col_names.push_back(r_name); }
    // need to iterate through valueTopos2 to get common data
    std::vector<decltype(tuple_cat_val)> new_tuple_vec;
    for (const auto& iter: valueTopos2) {
        T key = iter.first;
        size_t pos = iter.second;
        std::tuple<InnerTypes2...> right_tuple = boost::numeric::ublas::for_each_in_tuple(std::tuple<InnerTypes2...>{}, &other, colnamesr, pos);
        auto tmp_range = valueTopos1.equal_range(key);
        for (auto i = tmp_range.first; i != tmp_range.second; ++i) {
            size_t l_pos = i->second;
            std::tuple<InnerTypes1...> left_tuple = boost::numeric::ublas::for_each_in_tuple(std::tuple<InnerTypes1...>{}, this, colnamesl, l_pos);
            auto combined_tuple = std::tuple_cat(left_tuple, right_tuple);
            new_tuple_vec.push_back(combined_tuple);
        }
    }
    return *boost::numeric::ublas::make_from_tuples(new_tuple_vec, col_names, tuple_cat_val);
}
template<class... Types>
template<typename T,
                    typename... Types2, 
                    template<class...> class TypeLists1, typename... InnerTypes1, 
                    template<class...> class TypeLists2, typename... InnerTypes2>
auto data_frame<Types...>::combine_left(const data_frame<Types2...>& other, 
    const std::string& col_name, 
    TypeLists1<InnerTypes1...>, const std::vector<std::string>& colnamesl, 
    TypeLists2<InnerTypes2...>, const std::vector<std::string>& colnamesr) {
    static_assert(((std::is_same_v<T, Types> || ...)), "T type doesn't belong to common types");
    static_assert(((std::is_same_v<T, Types2> || ...)), "T type doesn't belong to common types");
    assert(sizeof...(InnerTypes1) == colnamesl.size());
    assert(sizeof...(InnerTypes2) == colnamesr.size());
    using type_collection_l = typename type_list<Types...>::types;
    using type_collection_r = typename type_list<Types2...>::types;
    auto merge_type_collection = merge_types(type_collection_l{}, type_collection_r{});
    int llen = get_cur_rows();
    int rlen = other.get_cur_rows();
    std::multimap<T, size_t> valueTopos1;
    std::multimap<T, size_t> valueTopos2;
    // Must iterate twice to get the number of rows in result data frame
    for (int i = 0; i < llen; i++)
        valueTopos1.insert({get_c<T>(col_name, i), i});
    for (int j = 0; j < rlen; j++) {
        const T& val = other.data_frame<Types2...>::template get_c<T>(col_name, j);
        if (valueTopos1.count(val))
            valueTopos2.insert({val, j});
    }
    // get concated tuple type and names
    auto tuple_cat_val = std::tuple_cat(std::tuple<InnerTypes1...>{}, std::tuple<InnerTypes2...>{});
    std::vector<std::string> col_names;
    for (const auto& l_name: colnamesl) { col_names.push_back(l_name); }
    for (const auto& r_name: colnamesr) { col_names.push_back(r_name); }
    // need to iterate through valueTopos2 to get common data
    std::vector<decltype(tuple_cat_val)> new_tuple_vec;
    for (const auto& iter: valueTopos2) {
        T key = iter.first;
        size_t pos = iter.second;
        std::tuple<InnerTypes2...> right_tuple = boost::numeric::ublas::for_each_in_tuple(std::tuple<InnerTypes2...>{}, &other, colnamesr, pos);
        auto tmp_range = valueTopos1.equal_range(key);
        for (auto i = tmp_range.first; i != tmp_range.second; ++i) {
            size_t l_pos = i->second;
            std::tuple<InnerTypes1...> left_tuple = boost::numeric::ublas::for_each_in_tuple(std::tuple<InnerTypes1...>{}, this, colnamesl, l_pos);
            auto combined_tuple = std::tuple_cat(left_tuple, right_tuple);
            new_tuple_vec.push_back(combined_tuple);
        }
    }
    for (const auto& iter: valueTopos1) {
        T key = iter.first;
        size_t pos = iter.second;
        if (!valueTopos2.count(key)) {
            std::tuple<InnerTypes2...> right_tuple = {};
            std::get<T>(right_tuple) = key;
            std::tuple<InnerTypes1...> left_tuple = boost::numeric::ublas::for_each_in_tuple(std::tuple<InnerTypes1...>{}, this, colnamesl, pos);
            auto combined_tuple = std::tuple_cat(left_tuple, right_tuple);
            new_tuple_vec.push_back(combined_tuple);
        }
    }
    return *boost::numeric::ublas::make_from_tuples(new_tuple_vec, col_names, tuple_cat_val);
}
template<class... Types>
template<typename T,
                    typename... Types2, 
                    template<class...> class TypeLists1, typename... InnerTypes1, 
                    template<class...> class TypeLists2, typename... InnerTypes2>
auto data_frame<Types...>::combine_right(const data_frame<Types2...>& other, 
    const std::string& col_name, 
    TypeLists1<InnerTypes1...>, const std::vector<std::string>& colnamesl, 
    TypeLists2<InnerTypes2...>, const std::vector<std::string>& colnamesr) {
    static_assert(((std::is_same_v<T, Types> || ...)), "T type doesn't belong to common types");
    static_assert(((std::is_same_v<T, Types2> || ...)), "T type doesn't belong to common types");
    assert(sizeof...(InnerTypes1) == colnamesl.size());
    assert(sizeof...(InnerTypes2) == colnamesr.size());
    using type_collection_l = typename type_list<Types...>::types;
    using type_collection_r = typename type_list<Types2...>::types;
    auto merge_type_collection = merge_types(type_collection_l{}, type_collection_r{});
    int llen = get_cur_rows();
    int rlen = other.get_cur_rows();
    std::multimap<T, size_t> valueTopos1;
    std::multimap<T, size_t> valueTopos2;
    std::multimap<T, size_t> valsNotInLeft;
    // Must iterate twice to get the number of rows in result data frame
    for (int i = 0; i < llen; i++)
        valueTopos1.insert({get_c<T>(col_name, i), i});
    for (int j = 0; j < rlen; j++) {
        const T& val = other.data_frame<Types2...>::template get_c<T>(col_name, j);
        if (valueTopos1.count(val))
            valueTopos2.insert({val, j});
        else {
            valsNotInLeft.insert({val, j});
        }
    }
    // get concated tuple type and names
    auto tuple_cat_val = std::tuple_cat(std::tuple<InnerTypes1...>{}, std::tuple<InnerTypes2...>{});
    std::vector<std::string> col_names;
    for (const auto& l_name: colnamesl) { col_names.push_back(l_name); }
    for (const auto& r_name: colnamesr) { col_names.push_back(r_name); }
    // need to iterate through valueTopos2 to get common data
    std::vector<decltype(tuple_cat_val)> new_tuple_vec;
    for (const auto& iter: valueTopos2) {
        T key = iter.first;
        size_t pos = iter.second;
        std::tuple<InnerTypes2...> right_tuple = boost::numeric::ublas::for_each_in_tuple(std::tuple<InnerTypes2...>{}, &other, colnamesr, pos);
        auto tmp_range = valueTopos1.equal_range(key);
        for (auto i = tmp_range.first; i != tmp_range.second; ++i) {
            size_t l_pos = i->second;
            std::tuple<InnerTypes1...> left_tuple = boost::numeric::ublas::for_each_in_tuple(std::tuple<InnerTypes1...>{}, this, colnamesl, l_pos);
            auto combined_tuple = std::tuple_cat(left_tuple, right_tuple);
            new_tuple_vec.push_back(combined_tuple);
        }
    }
    // adding remaining right rows which doesn't include in the left `data_frame`
    for (const auto& iter: valsNotInLeft) {
        T key = iter.first;
        size_t pos = iter.second;
        std::tuple<InnerTypes1...> left_tuple = {};
        std::get<T>(left_tuple) = key;
        std::tuple<InnerTypes2...> right_tuple = boost::numeric::ublas::for_each_in_tuple(std::tuple<InnerTypes2...>{}, &other, colnamesr, pos);
        auto combined_tuple = std::tuple_cat(left_tuple, right_tuple);
        new_tuple_vec.push_back(combined_tuple);
    }
    return *boost::numeric::ublas::make_from_tuples(new_tuple_vec, col_names, tuple_cat_val);
}
template<class... Types>
template<typename T,
                    typename... Types2, 
                    template<class...> class TypeLists1, typename... InnerTypes1, 
                    template<class...> class TypeLists2, typename... InnerTypes2>
auto data_frame<Types...>::combine_full(const data_frame<Types2...>& other, 
    const std::string& col_name, 
    TypeLists1<InnerTypes1...>, const std::vector<std::string>& colnamesl, 
    TypeLists2<InnerTypes2...>, const std::vector<std::string>& colnamesr) {
    static_assert(((std::is_same_v<T, Types> || ...)), "T type doesn't belong to common types");
    static_assert(((std::is_same_v<T, Types2> || ...)), "T type doesn't belong to common types");
    assert(sizeof...(InnerTypes1) == colnamesl.size());
    assert(sizeof...(InnerTypes2) == colnamesr.size());
    using type_collection_l = typename type_list<Types...>::types;
    using type_collection_r = typename type_list<Types2...>::types;
    auto merge_type_collection = merge_types(type_collection_l{}, type_collection_r{});
    int llen = get_cur_rows();
    int rlen = other.get_cur_rows();
    std::multimap<T, size_t> valueTopos1;
    std::multimap<T, size_t> valueTopos2;
    std::multimap<T, size_t> valsNotInLeft;
    // Must iterate twice to get the number of rows in result data frame
    for (int i = 0; i < llen; i++)
        valueTopos1.insert({get_c<T>(col_name, i), i});
    for (int j = 0; j < rlen; j++) {
        const T& val = other.data_frame<Types2...>::template get_c<T>(col_name, j);
        if (valueTopos1.count(val))
            valueTopos2.insert({val, j});
        else {
            valsNotInLeft.insert({val, j});
        }
    }
    // get concated tuple type and names
    auto tuple_cat_val = std::tuple_cat(std::tuple<InnerTypes1...>{}, std::tuple<InnerTypes2...>{});
    std::vector<std::string> col_names;
    for (const auto& l_name: colnamesl) { col_names.push_back(l_name); }
    for (const auto& r_name: colnamesr) { col_names.push_back(r_name); }
    // need to iterate through valueTopos2 to get common data
    std::vector<decltype(tuple_cat_val)> new_tuple_vec;
    for (const auto& iter: valueTopos2) {
        T key = iter.first;
        size_t pos = iter.second;
        std::tuple<InnerTypes2...> right_tuple = boost::numeric::ublas::for_each_in_tuple(std::tuple<InnerTypes2...>{}, &other, colnamesr, pos);
        auto tmp_range = valueTopos1.equal_range(key);
        for (auto i = tmp_range.first; i != tmp_range.second; ++i) {
            size_t l_pos = i->second;
            std::tuple<InnerTypes1...> left_tuple = boost::numeric::ublas::for_each_in_tuple(std::tuple<InnerTypes1...>{}, this, colnamesl, l_pos);
            auto combined_tuple = std::tuple_cat(left_tuple, right_tuple);
            new_tuple_vec.push_back(combined_tuple);
        }
    }
    // adding remaining left rows which doesn't include in the right `data_frame`
    for (const auto& iter: valueTopos1) {
        T key = iter.first;
        size_t pos = iter.second;
        if (!valueTopos2.count(key)) {
            std::tuple<InnerTypes2...> right_tuple = {};
            std::get<T>(right_tuple) = key;
            std::tuple<InnerTypes1...> left_tuple = boost::numeric::ublas::for_each_in_tuple(std::tuple<InnerTypes1...>{}, this, colnamesl, pos);
            auto combined_tuple = std::tuple_cat(left_tuple, right_tuple);
            new_tuple_vec.push_back(combined_tuple);
        }
    }
    // adding remaining right rows which doesn't include in the left `data_frame`
    for (const auto& iter: valsNotInLeft) {
        T key = iter.first;
        size_t pos = iter.second;
        std::tuple<InnerTypes1...> left_tuple = {};
        std::get<T>(left_tuple) = key;
        std::tuple<InnerTypes2...> right_tuple = boost::numeric::ublas::for_each_in_tuple(std::tuple<InnerTypes2...>{}, &other, colnamesr, pos);
        auto combined_tuple = std::tuple_cat(left_tuple, right_tuple);
        new_tuple_vec.push_back(combined_tuple);
    }
    return *boost::numeric::ublas::make_from_tuples(new_tuple_vec, col_names, tuple_cat_val);
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
/** @brief inner join two data frames on specific column 
* 
* @tparam T the type of the column to be joined
* 
* @tparam Types1... the template argument for left data_frame
*
* @tparam Types2... the template argument for right data_frame
* 
* @tparam TypeLists1 the type container for left data_frame 
* 
* @tparam InnerTypes1... the types for left data_frame
* 
* @tparam TypeLists2 the type conatiner for right data_frame
* 
* @tparam InnerTypes2... the types for right data_frame
* 
* @param l the data_frame on left 
*
* @param r the data_frame on right
* 
* @param col_name the name of column to be joined on
* 
* @param TypeLists1<InnerTypes1...> used to deduct tuple type for current data_frame
* 
* @param colnamesl the corresponding column names for each type position in TypeLists1<InnerTypes1...>
* 
* @param TypeLists2<InnerTypes2...> used to deduct tuple type for second data_frame
* 
* @param colnamesr the corresponding column names for each type position in TypeLists2<InnerTypes2...>
*/      
template<typename T,
                    typename... Types1, 
                    typename... Types2, 
                    template<class...> class TypeLists1, typename... InnerTypes1, 
                    template<class...> class TypeLists2, typename... InnerTypes2>
auto combine_inner(const data_frame<Types1...>& l, const data_frame<Types2...>& r, 
    const std::string& col_name, 
    TypeLists1<InnerTypes1...>, const std::vector<std::string>& colnamesl, 
    TypeLists2<InnerTypes2...>, const std::vector<std::string>& colnamesr) {
    static_assert(((std::is_same_v<T, Types1> || ...)), "T type doesn't belong to common types");
    static_assert(((std::is_same_v<T, Types2> || ...)), "T type doesn't belong to common types");
    assert(sizeof...(InnerTypes1) == colnamesl.size());
    assert(sizeof...(InnerTypes2) == colnamesr.size());
    using type_collection_l = typename type_list<Types1...>::types;
    using type_collection_r = typename type_list<Types2...>::types;
    auto merge_type_collection = merge_types(type_collection_l{}, type_collection_r{});
    int llen = l.get_cur_rows();
    int rlen = r.get_cur_rows();
    std::multimap<T, size_t> valueTopos1;
    std::multimap<T, size_t> valueTopos2;
    // Must iterate twice to get the number of rows in result data frame
    for (int i = 0; i < llen; i++)
        valueTopos1.insert({l.data_frame<Types1...>::template get_c<T>(col_name, i), i});
    for (int j = 0; j < rlen; j++) {
        const T& val = r.data_frame<Types2...>::template get_c<T>(col_name, j);
        if (valueTopos1.count(val))
            valueTopos2.insert({val, j});
    }
    // get concated tuple type and names
    auto tuple_cat_val = std::tuple_cat(std::tuple<InnerTypes1...>{}, std::tuple<InnerTypes2...>{});
    std::vector<std::string> col_names;
    for (const auto& l_name: colnamesl) { col_names.push_back(l_name); }
    for (const auto& r_name: colnamesr) { col_names.push_back(r_name); }
    // need to iterate through valueTopos2 to get common data
    std::vector<decltype(tuple_cat_val)> new_tuple_vec;
    for (const auto& iter: valueTopos2) {
        T key = iter.first;
        size_t pos = iter.second;
        std::tuple<InnerTypes2...> right_tuple = for_each_in_tuple(std::tuple<InnerTypes2...>{}, &r, colnamesr, pos);
        auto tmp_range = valueTopos1.equal_range(key);
        for (auto i = tmp_range.first; i != tmp_range.second; ++i) {
            size_t l_pos = i->second;
            std::tuple<InnerTypes1...> left_tuple = for_each_in_tuple(std::tuple<InnerTypes1...>{}, &l, colnamesl, l_pos);
            auto combined_tuple = std::tuple_cat(left_tuple, right_tuple);
            new_tuple_vec.push_back(combined_tuple);
        }
    }
    auto new_df = make_from_tuples(new_tuple_vec, col_names, tuple_cat_val);
    return new_df;
}
/** @brief left join two data frames on specific column 
* 
* @tparam T the type of the column to be joined
* 
* @tparam Types1... the template argument for left data_frame
*
* @tparam Types2... the template argument for right data_frame
* 
* @tparam TypeLists1 the type container for left data_frame 
* 
* @tparam InnerTypes1... the types for left data_frame
* 
* @tparam TypeLists2 the type conatiner for right data_frame
* 
* @tparam InnerTypes2... the types for right data_frame
* 
* @param l the data_frame on left 
*
* @param r the data_frame on right
* 
* @param col_name the name of column to be joined on
* 
* @param TypeLists1<InnerTypes1...> used to deduct tuple type for current data_frame
* 
* @param colnamesl the corresponding column names for each type position in TypeLists1<InnerTypes1...>
* 
* @param TypeLists2<InnerTypes2...> used to deduct tuple type for second data_frame
* 
* @param colnamesr the corresponding column names for each type position in TypeLists2<InnerTypes2...>
*/  
template<typename T,
                    typename... Types1, 
                    typename... Types2, 
                    template<class...> class TypeLists1, typename... InnerTypes1, 
                    template<class...> class TypeLists2, typename... InnerTypes2>
auto combine_left(const data_frame<Types1...>& l, const data_frame<Types2...>& r, 
    const std::string& col_name, 
    TypeLists1<InnerTypes1...>, const std::vector<std::string>& colnamesl, 
    TypeLists2<InnerTypes2...>, const std::vector<std::string>& colnamesr) {
    static_assert(((std::is_same_v<T, Types1> || ...)), "T type doesn't belong to common types");
    static_assert(((std::is_same_v<T, Types2> || ...)), "T type doesn't belong to common types");
    assert(sizeof...(InnerTypes1) == colnamesl.size());
    assert(sizeof...(InnerTypes2) == colnamesr.size());
    using type_collection_l = typename type_list<Types1...>::types;
    using type_collection_r = typename type_list<Types2...>::types;
    auto merge_type_collection = merge_types(type_collection_l{}, type_collection_r{});
    int llen = l.get_cur_rows();
    int rlen = r.get_cur_rows();
    std::multimap<T, size_t> valueTopos1;
    std::multimap<T, size_t> valueTopos2;
    // Must iterate twice to get the number of rows in result data frame
    for (int i = 0; i < llen; i++)
        valueTopos1.insert({l.data_frame<Types1...>::template get_c<T>(col_name, i), i});
    for (int j = 0; j < rlen; j++) {
        const T& val = r.data_frame<Types2...>::template get_c<T>(col_name, j);
        if (valueTopos1.count(val))
            valueTopos2.insert({val, j});
    }
    // get concated tuple type and names
    auto tuple_cat_val = std::tuple_cat(std::tuple<InnerTypes1...>{}, std::tuple<InnerTypes2...>{});
    std::vector<std::string> col_names;
    for (const auto& l_name: colnamesl) { col_names.push_back(l_name); }
    for (const auto& r_name: colnamesr) { col_names.push_back(r_name); }
    // need to iterate through valueTopos2 to get common data
    std::vector<decltype(tuple_cat_val)> new_tuple_vec;
    for (const auto& iter: valueTopos2) {
        T key = iter.first;
        size_t pos = iter.second;
        std::tuple<InnerTypes2...> right_tuple = for_each_in_tuple(std::tuple<InnerTypes2...>{}, &r, colnamesr, pos);
        auto tmp_range = valueTopos1.equal_range(key);
        for (auto i = tmp_range.first; i != tmp_range.second; ++i) {
            size_t l_pos = i->second;
            std::tuple<InnerTypes1...> left_tuple = for_each_in_tuple(std::tuple<InnerTypes1...>{}, &l, colnamesl, l_pos);
            auto combined_tuple = std::tuple_cat(left_tuple, right_tuple);
            new_tuple_vec.push_back(combined_tuple);
        }
    }
    // adding remaining left rows which doesn't include in the right `data_frame`
    for (const auto& iter: valueTopos1) {
        T key = iter.first;
        size_t pos = iter.second;
        if (!valueTopos2.count(key)) {
            std::tuple<InnerTypes2...> right_tuple = {};
            std::get<T>(right_tuple) = key;
            std::tuple<InnerTypes1...> left_tuple = for_each_in_tuple(std::tuple<InnerTypes1...>{}, &l, colnamesl, pos);
            auto combined_tuple = std::tuple_cat(left_tuple, right_tuple);
            new_tuple_vec.push_back(combined_tuple);
        }
    }
    auto new_df = make_from_tuples(new_tuple_vec, col_names, tuple_cat_val);
    return new_df;
}
/** @brief right join two data frames on specific column 
* 
* @tparam T the type of the column to be joined
* 
* @tparam Types1... the template argument for left data_frame
*
* @tparam Types2... the template argument for right data_frame
* 
* @tparam TypeLists1 the type container for left data_frame 
* 
* @tparam InnerTypes1... the types for left data_frame
* 
* @tparam TypeLists2 the type conatiner for right data_frame
* 
* @tparam InnerTypes2... the types for right data_frame
* 
* @param l the data_frame on left 
*
* @param r the data_frame on right
* 
* @param col_name the name of column to be joined on
* 
* @param TypeLists1<InnerTypes1...> used to deduct tuple type for current data_frame
* 
* @param colnamesl the corresponding column names for each type position in TypeLists1<InnerTypes1...>
* 
* @param TypeLists2<InnerTypes2...> used to deduct tuple type for second data_frame
* 
* @param colnamesr the corresponding column names for each type position in TypeLists2<InnerTypes2...>
*/  
template<typename T,
                    typename... Types1, 
                    typename... Types2, 
                    template<class...> class TypeLists1, typename... InnerTypes1, 
                    template<class...> class TypeLists2, typename... InnerTypes2>
auto combine_right(const data_frame<Types1...>& l, const data_frame<Types2...>& r, 
    const std::string& col_name, 
    TypeLists1<InnerTypes1...>, const std::vector<std::string>& colnamesl, 
    TypeLists2<InnerTypes2...>, const std::vector<std::string>& colnamesr) {
    return combine_left<T>(r, l, col_name, TypeLists2<InnerTypes2...>{}, colnamesr, TypeLists1<InnerTypes1...>{}, colnamesl);
}
/** @brief full join two data frames on specific column 
* 
* @tparam T the type of the column to be joined
* 
* @tparam Types1... the template argument for left data_frame
*
* @tparam Types2... the template argument for right data_frame
* 
* @tparam TypeLists1 the type container for left data_frame 
* 
* @tparam InnerTypes1... the types for left data_frame
* 
* @tparam TypeLists2 the type conatiner for right data_frame
* 
* @tparam InnerTypes2... the types for right data_frame
* 
* @param l the data_frame on left 
*
* @param r the data_frame on right
* 
* @param col_name the name of column to be joined on
* 
* @param TypeLists1<InnerTypes1...> used to deduct tuple type for current data_frame
* 
* @param colnamesl the corresponding column names for each type position in TypeLists1<InnerTypes1...>
* 
* @param TypeLists2<InnerTypes2...> used to deduct tuple type for second data_frame
* 
* @param colnamesr the corresponding column names for each type position in TypeLists2<InnerTypes2...>
*/  
template<typename T,
                    typename... Types1, 
                    typename... Types2, 
                    template<class...> class TypeLists1, typename... InnerTypes1, 
                    template<class...> class TypeLists2, typename... InnerTypes2>
auto combine_full(const data_frame<Types1...>& l, const data_frame<Types2...>& r, 
    const std::string& col_name, 
    TypeLists1<InnerTypes1...>, const std::vector<std::string>& colnamesl, 
    TypeLists2<InnerTypes2...>, const std::vector<std::string>& colnamesr) {
    static_assert(((std::is_same_v<T, Types1> || ...)), "T type doesn't belong to common types");
    static_assert(((std::is_same_v<T, Types2> || ...)), "T type doesn't belong to common types");
    assert(sizeof...(InnerTypes1) == colnamesl.size());
    assert(sizeof...(InnerTypes2) == colnamesr.size());
    using type_collection_l = typename type_list<Types1...>::types;
    using type_collection_r = typename type_list<Types2...>::types;
    auto merge_type_collection = merge_types(type_collection_l{}, type_collection_r{});
    int llen = l.get_cur_rows();
    int rlen = r.get_cur_rows();
    std::multimap<T, size_t> valueTopos1;
    std::multimap<T, size_t> valueTopos2;
    std::multimap<T, size_t> valsNotInLeft;
    // Must iterate twice to get the number of rows in result data frame
    for (int i = 0; i < llen; i++)
        valueTopos1.insert({l.data_frame<Types1...>::template get_c<T>(col_name, i), i});
    for (int j = 0; j < rlen; j++) {
        const T& val = r.data_frame<Types2...>::template get_c<T>(col_name, j);
        if (valueTopos1.count(val))
            valueTopos2.insert({val, j});
        else {
            valsNotInLeft.insert({val, j});
        }
    }
    // get concated tuple type and names
    auto tuple_cat_val = std::tuple_cat(std::tuple<InnerTypes1...>{}, std::tuple<InnerTypes2...>{});
    std::vector<std::string> col_names;
    for (const auto& l_name: colnamesl) { col_names.push_back(l_name); }
    for (const auto& r_name: colnamesr) { col_names.push_back(r_name); }
    // need to iterate through valueTopos2 to get common data
    std::vector<decltype(tuple_cat_val)> new_tuple_vec;
    for (const auto& iter: valueTopos2) {
        T key = iter.first;
        size_t pos = iter.second;
        std::tuple<InnerTypes2...> right_tuple = for_each_in_tuple(std::tuple<InnerTypes2...>{}, &r, colnamesr, pos);
        auto tmp_range = valueTopos1.equal_range(key);
        for (auto i = tmp_range.first; i != tmp_range.second; ++i) {
            size_t l_pos = i->second;
            std::tuple<InnerTypes1...> left_tuple = for_each_in_tuple(std::tuple<InnerTypes1...>{}, &l, colnamesl, l_pos);
            auto combined_tuple = std::tuple_cat(left_tuple, right_tuple);
            new_tuple_vec.push_back(combined_tuple);
        }
    }
    // adding remaining left rows which doesn't include in the right `data_frame`
    for (const auto& iter: valueTopos1) {
        T key = iter.first;
        size_t pos = iter.second;
        if (!valueTopos2.count(key)) {
            std::tuple<InnerTypes2...> right_tuple = {};
            std::get<T>(right_tuple) = key;
            std::tuple<InnerTypes1...> left_tuple = for_each_in_tuple(std::tuple<InnerTypes1...>{}, &l, colnamesl, pos);
            auto combined_tuple = std::tuple_cat(left_tuple, right_tuple);
            new_tuple_vec.push_back(combined_tuple);
        }
    }
    // adding remaining right rows which doesn't include in the left `data_frame`
    for (const auto& iter: valsNotInLeft) {
        T key = iter.first;
        size_t pos = iter.second;
        std::tuple<InnerTypes1...> left_tuple = {};
        std::get<T>(left_tuple) = key;
        std::tuple<InnerTypes2...> right_tuple = for_each_in_tuple(std::tuple<InnerTypes2...>{}, &r, colnamesr, pos);
        auto combined_tuple = std::tuple_cat(left_tuple, right_tuple);
        new_tuple_vec.push_back(combined_tuple);
    }
    auto new_df = make_from_tuples(new_tuple_vec, col_names, tuple_cat_val);
    return new_df;
}
/** @brief row intersect of two data_frames with the same type
*
* @tparam Types... the template argument for data_frames
* 
* @tparam TypeLists the type container for data_frame 
* 
* @tparam InnerTypes... the types for data_frame tuple
* 
* @param l the data_frame on left 
*
* @param r the data_frame on right
* 
* @param TypeLists<InnerTypes...> used to deduct tuple type for current data_frame
* 
* @param colnames the corresponding column names for each type position in TypeLists<InnerTypes...>
*/  
template<typename... Types, template<class...> class TypeLists, typename... InnerTypes>
auto intersect(const data_frame<Types...>& l, const data_frame<Types...>& r, 
    TypeLists<InnerTypes...>, const std::vector<std::string>& colnames) {
    std::vector<std::string> col_names;
    for (const auto& name: colnames) col_names.push_back(name);
    std::set<std::tuple<InnerTypes...>> left_tuples_set;
    std::set<std::tuple<InnerTypes...>> right_tuples_set;
    int llen = l.get_cur_rows();
    int rlen = r.get_cur_rows();
    for (int i = 0; i < llen; i++) {
        std::tuple<InnerTypes...> left_tuple = for_each_in_tuple(std::tuple<InnerTypes...>{}, &l, colnames, i);
        left_tuples_set.insert(left_tuple);
    }
    for (int i = 0; i < rlen; i++) {
        std::tuple<InnerTypes...> right_tuple = for_each_in_tuple(std::tuple<InnerTypes...>{}, &r, colnames, i);
        right_tuples_set.insert(right_tuple);       
    }
    std::vector<std::tuple<InnerTypes...>> interset_tuples;
    for (const auto& iter: left_tuples_set) {
        if (right_tuples_set.count(iter)) {
            interset_tuples.push_back(iter);
        }
    }
    return make_from_tuples(interset_tuples, col_names, TypeLists<InnerTypes...>{});
}
/** @brief the tuples containing in l data_frame but don't exist in r data_frame
*
* @tparam Types... the template argument for data_frames
* 
* @tparam TypeLists the type container for data_frame 
* 
* @tparam InnerTypes... the types for data_frame tuple
* 
* @param l the data_frame on left 
*
* @param r the data_frame on right
* 
* @param TypeLists<InnerTypes...> used to deduct tuple type for current data_frame
* 
* @param colnames the corresponding column names for each type position in TypeLists<InnerTypes...>
*/  
template<typename... Types, template<class...> class TypeLists, typename... InnerTypes>
auto setdiff(const data_frame<Types...>& l, const data_frame<Types...>& r, 
    TypeLists<InnerTypes...>, const std::vector<std::string>& colnames) {
    std::vector<std::string> col_names;
    for (const auto& name: colnames) col_names.push_back(name);
    std::set<std::tuple<InnerTypes...>> left_tuples_set;
    std::set<std::tuple<InnerTypes...>> right_tuples_set;
    int llen = l.get_cur_rows();
    int rlen = r.get_cur_rows();
    for (int i = 0; i < llen; i++) {
        std::tuple<InnerTypes...> left_tuple = for_each_in_tuple(std::tuple<InnerTypes...>{}, &l, colnames, i);
        left_tuples_set.insert(left_tuple);
    }
    for (int i = 0; i < rlen; i++) {
        std::tuple<InnerTypes...> right_tuple = for_each_in_tuple(std::tuple<InnerTypes...>{}, &r, colnames, i);
        right_tuples_set.insert(right_tuple);       
    }
    std::vector<std::tuple<InnerTypes...>> interset_tuples;
    for (const auto& iter: left_tuples_set) {
        if (!right_tuples_set.count(iter)) {
            interset_tuples.push_back(iter);
        }
    }
    for (const auto& iter: right_tuples_set) {
        if (!left_tuples_set.count(iter)) {
            interset_tuples.push_back(iter);
        }
    }
    return make_from_tuples(interset_tuples, col_names, TypeLists<InnerTypes...>{});
}
/** @brief the tuples containing in l data_frame or exist in r data_frame
*
* @tparam Types... the template argument for data_frames
* 
* @tparam TypeLists the type container for data_frame 
* 
* @tparam InnerTypes... the types for data_frame tuple
* 
* @param l the data_frame on left 
*
* @param r the data_frame on right
* 
* @param TypeLists<InnerTypes...> used to deduct tuple type for current data_frame
* 
* @param colnames the corresponding column names for each type position in TypeLists<InnerTypes...>
*/  
template<typename... Types, template<class...> class TypeLists, typename... InnerTypes>
auto setunion(const data_frame<Types...>& l, const data_frame<Types...>& r, 
    TypeLists<InnerTypes...>, const std::vector<std::string>& colnames) {
    std::vector<std::string> col_names;
    for (const auto& name: colnames) col_names.push_back(name);
    std::set<std::tuple<InnerTypes...>> left_tuples_set;
    std::set<std::tuple<InnerTypes...>> right_tuples_set;
    std::set<std::tuple<InnerTypes...>> complete_tuples_set;
    int llen = l.get_cur_rows();
    int rlen = r.get_cur_rows();
    for (int i = 0; i < llen; i++) {
        std::tuple<InnerTypes...> left_tuple = for_each_in_tuple(std::tuple<InnerTypes...>{}, &l, colnames, i);
        left_tuples_set.insert(left_tuple);
        complete_tuples_set.insert(left_tuple);
    }
    for (int i = 0; i < rlen; i++) {
        std::tuple<InnerTypes...> right_tuple = for_each_in_tuple(std::tuple<InnerTypes...>{}, &r, colnames, i);
        right_tuples_set.insert(right_tuple);      
        complete_tuples_set.insert(right_tuple); 
    }
    std::vector<std::tuple<InnerTypes...>> interset_tuples(complete_tuples_set.begin(), complete_tuples_set.end());
    return make_from_tuples(interset_tuples, col_names, TypeLists<InnerTypes...>{});
}
/** @brief data_frame_view represents a view of data_frame, and it only contains row index in original data_frame
 * 
 * @tparam Types... represent a non-repeated types from all data_frame
 */
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
   /** @brief change existing value for specific indexes
    * 
    * @tparam F the functor type for applying
    * 
    * @param index the index for using the applying the computation
    * 
    * @param f the functor to compute new value
    */   
    template<typename F>
    data_frame_view<Types...>& apply_with_index(const std::vector<int>& index, F f) {
        data_frame_ptr->apply_with_index(index, f);
        return *this;
    }
    /** @brief print for specific indexes
    * 
    * @param index the index for printing
    */   
    void print_with_index(const std::vector<int>& index) {
        data_frame_ptr->print_with_index(index);
    }
    /** @brief print for specific indexes
    * 
    * @param index the range for printing
    */   
    void print_with_range(const range& index) {
        data_frame_ptr->print_with_range(index);
    }
    /** @brief print for specific indexes
    * 
    * @param index the slice for printing
    */   
    void print_with_slice(const slice& index) {
        data_frame_ptr->print_with_slice(index);
    }
   /** @brief create a view with new row orders after sort
    * 
    * @tparam T the column type to be filtered
    * 
    * @param col_name the column name applying the condition
    */   
    template<typename T>
    data_frame_view<Types...>& sort(const std::string& col_name) {
        static_assert(((std::is_same_v<T, Types> || ...)), "Type doesn't match to data_frame_view");
        data_frame_ptr->data_frame<Types...>::template sort<T>(col_name);
        return *this;
    }
   /** @brief create a view with new row orders after sort
    * 
    * @tparam T the column type to be filtered
    * 
    * @tparam F a functor for condition
    * 
    * @param col_name the column name applying the condition
    * 
    * @param f a functor for condition
    */   
    template<typename T, typename F>
    data_frame_view<Types...>& sort(const std::string& col_name, F f) {
        static_assert(((std::is_same_v<T, Types> || ...)), "Type doesn't match to data_frame_view");
        data_frame_ptr->data_frame<Types...>::template sort<T>(col_name, f);
        return *this;
    }
    /** @brief create a view with rows only satisfying a condition 
    * 
    * @tparam T the column type to be filtered
    * 
    * @tparam F a functor for condition
    * 
    * @param col_name the column name applying the condition
    * 
    * @param f a functor for condition
    */   
    template<typename T, typename F>
    data_frame_view<Types...>& select(const std::string& col_name, F f) {
        static_assert(((std::is_same_v<T, Types> || ...)), "Type doesn't match to data_frame_view");
        data_frame_ptr->data_frame<Types...>::template select<T>(col_name, f);
        return *this;
    }
    /** @brief return data with at pos row in col_name position
    *
    * @tparam T the type for col_name column 
    *  
    * @param pos row position
    */   
    template<typename T>
    T& get(const std::string& col_name, size_t pos) {
        static_assert(((std::is_same_v<T, Types> || ...)), "Type doesn't match to data_frame_view");
        return data_frame_ptr->data_frame<Types...>::template get<T>(col_name, pos);
    }
    /** @brief return const data with at pos row in col_name position
    *
    * @tparam T the type for col_name column 
    *  
    * @param pos row position
    */   
    template<typename T>
    const T& get_c(const std::string& col_name, size_t pos) {
        static_assert(((std::is_same_v<T, Types> || ...)), "Type doesn't match to data_frame_view");
        return data_frame_ptr->data_frame<Types...>::template get_c<T>(col_name, pos);
    }
    size_t get_cur_rows() {
        return internal_index.size();
    }
    size_t get_cur_cols() {
        return data_frame_ptr->get_cur_cols();
    }
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