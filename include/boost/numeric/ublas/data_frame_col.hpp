#ifndef _BOOST_UBLAS_DATA_FRAME_COL_	
#define _BOOST_UBLAS_DATA_FRAME_COL_
#include <boost/mp11/algorithm.hpp>
#include <boost/numeric/ublas/vector.hpp>
#include <unordered_map>
#include <functional>
#include <vector>
#include <variant>
namespace boost { namespace numeric { namespace ublas {	
/** @brief a list containing different types,@code type_list::types @endcode represents a non-repeated types
 */
template<typename... Typelists>
struct type_list {
    using original_types = std::tuple<Typelists...>;
    using types = boost::mp11::mp_unique<original_types>;
};
/** @brief compute the types after merge two @code type_list @endcode
 * the return value type represents the new type
 */
template <template <typename...> class TypeLists1, typename... Types1, 
          template <typename...> class TypeLists2, typename... Types2>
auto merge_types(TypeLists1<Types1...> l, TypeLists2<Types2...> r) {
    auto new_tuple = std::tuple_cat(l, r);
    using types = boost::mp11::mp_unique<decltype(new_tuple)>;
    return types{};
}
/** @brief data_frame_col represents each column within one data_frame, and it's designed as a heterogenous container. 
 * One data_frrame_col instance can store different types, but it's only used to store a single type. 
 * Users can extract its content by using visitor pattern. 
 */
class data_frame_col {
public:
    template<typename T>
    using store_type = boost::numeric::ublas::vector<T>;
    /** @brief Build an empty data_frame_col
     *
     * @note current column is empty
     */
    data_frame_col() = default;
    /** @brief Build data_frame_col with @code col_name @endcode as column name containing data from @code col_vec @endcode
     *
    * @tparam T type of the objects stored in the data_frame_col
    * 
    * @param col_name name for this column
    * 
    * @param col_vec data stored in the @code data_frame_col @endcode
    */
    template<typename T> 
    data_frame_col(std::string col_name, const std::vector<T>& col_vec) {
        build_by_vec<T>(col_name, col_vec);
    }
    /** @brief Build data_frame_col from existing data_frame_col
    */   
    data_frame_col(const data_frame_col& _other) {
        *this = _other;
    }
    /** @brief Build data_frame_col with @code col_name @endcode as column name containing data from @code col_vec @endcode
     *
    * @tparam T type of the objects stored in the data_frame_col
    * 
    * @param col_name name for this column
    * 
    * @param col_vec data stored in the @code data_frame_col @endcode
    */
    template<typename T>
    void build_by_vec(std::string col_name, const std::vector<T>& other);
    /** @brief Get number of records in current @code data_frame_col @endcode
     *
    * @tparam T type of the objects stored in the data_frame_col
    */
    template<typename T> 
    int get_size() {
        if (vals<T>.count(this))
            return vals<T>[this].size();
        else return 0;
    }
    /** @brief Get a reference for data stored at index in @code data_frame_col @endcode
     *
    * @param index position within @code data_frame_col @endcode
    */
    template<typename T>
    T& at(size_t index) {
        // need to handle exception here
        return vals<T>[this][index];
    }
    /** @brief Get a const reference for data stored at index in @code data_frame_col @endcode
     *
    * @param index position within @code data_frame_col @endcode
    */
    template<typename T>
    const T& at(size_t index) const {
        // need to handle exception here
        return vals<T>[this][index];
    }
    /** @brief Copy content from another @code data_frame_col @endcode
     *
    * @param _other data_frame_col to be copied
    */
    data_frame_col& operator=(const data_frame_col& _other)
    {
        clear();
        clear_functions = _other.clear_functions;
        copy_functions = _other.copy_functions;
        size_functions = _other.size_functions;
        for (auto&& copy_function : copy_functions)
        {
            copy_function(_other, *this);
        }
        return *this;
    }
    ~data_frame_col() {
        clear();
    }
    /** @brief get underlying container from @code data_frame_col @endcode
    *
    * @tparam T type of the objects stored in the data_frame_col
    */
    template<typename T>
    store_type<T>& get_vector() {
        return vals<T>[this];
    }
    /** @brief get a const underlying container from @code data_frame_col @endcode
    *
    * @tparam T type of the objects stored in the data_frame_col
    */
    template<typename T>
    const store_type<T>& get_vector() const {
        return vals<T>[this];
    }
    /** @brief compute new value at specific position for specific column in @code data_frame_col @endcode
    *
    * @tparam F functor to compute new value, requires original value and column name
    * 
    * @tparam TypeLists a set of potential types, used as @code TypeLists<Types...> @endcode
    * 
    * @tparam Types... a typelists containing concrete types in @code TypeLists<Types...> @endcode
    * 
    * @param index the position to compute value
    * 
    * @param col_name name for current @code data_frame_col @endcode
    * 
    * @param f functor for computing new value
    * 
    * @param TypeLists<Types...> used to deduct types
    */
    template<typename F, template<class...> class TypeLists, typename... Types>
    void fill_data_at(int index, const std::string& col_name, F&& f, TypeLists<Types...>) {
        (..., [this, functor = std::move(f)](int i, const std::string& name) mutable {
            if (vals<Types>[this].size() > 0)
                functor(at<Types>(i), name);
        }(index, col_name));
    }
    /** @brief initialize values for @code data_frame_col @endcode
    *
    * @tparam F functor to initiliaze new value, requires original value to deduct type
    * 
    * @tparam TypeLists a set of potential types, used as @code TypeLists<Types...> @endcode
    * 
    * @tparam Types... a typelists containing concrete types in @code TypeLists<Types...> @endcode
    * 
    * @param f functor for initialize @code data_frame_col @endcode
    * 
    * @param TypeLists<Types...> used to deduct types
    */
    template<typename F, template<class...> class TypeLists, typename... Types>
    void initialize(F&& f, TypeLists<Types...>) {
         (..., [this, functor = std::move(f)]() mutable {
            if (vals<Types>[this].size() > 0) 
                functor(vals<Types>[this][0]);
         }());
    }
    /** @brief compute new value at specific position within @code data_frame_col @endcode
    *
    * @tparam F functor to compute new value, requires original value and column name
    * 
    * @tparam TypeLists a set of potential types, used as @code TypeLists<Types...> @endcode
    * 
    * @tparam Types... a typelists containing concrete types in @code TypeLists<Types...> @endcode
    * 
    * @param index the position to compute value
    * 
    * @param f functor for computing new value
    * 
    * @param TypeLists<Types...> used to deduct types
    */
    template<typename F, template<class...> class TypeLists, typename... Types>
    void apply_at(int index, F&& f, TypeLists<Types...>) {
        (..., [this, functor = std::move(f)](int i) {
            if (vals<Types>[this].size() > 0) 
                functor(vals<Types>[this][i]);
        }(index));
    }
    std::string col_name;
private:
    void clear() {
        for (auto&& clear_func : clear_functions) {
            clear_func(*this);
        }
    }
    template<class T>
    static std::unordered_map<const data_frame_col*, store_type<T>> vals;
    std::vector<std::function<void(data_frame_col&)>> clear_functions;
    std::vector<std::function<void(const data_frame_col&, data_frame_col&)>> copy_functions;
    std::vector<std::function<size_t(const data_frame_col&)>> size_functions;
};
template<class T>
std::unordered_map<const data_frame_col*, typename data_frame_col::store_type<T>> data_frame_col::vals;

template<typename T>
void data_frame_col::build_by_vec(std::string col_name, const std::vector<T>& other) {
    if (!vals<T>.count(this)) {
        clear_functions.emplace_back([](data_frame_col& _c){vals<T>.erase(&_c);});
        copy_functions.emplace_back([](const data_frame_col& _from, data_frame_col& _to) {
            vals<T>[&_to] = vals<T>[&_from];
        });
        size_functions.emplace_back([](const data_frame_col& _c){return vals<T>[&_c].size();});
    }
    int len = other.size();
    vals<T>[this] = store_type<T>(len);
    this->col_name = col_name;
    for (int i = 0; i < len; i++) {
        vals<T>[this](i) = other[i];
    }
}
}}}



#endif