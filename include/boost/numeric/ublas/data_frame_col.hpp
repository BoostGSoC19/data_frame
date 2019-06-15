#ifndef _BOOST_UBLAS_DATA_FRAME_COL_	
#define _BOOST_UBLAS_DATA_FRAME_COL_
#include <boost/numeric/ublas/vector.hpp>
#include <unordered_map>
#include <functional>
#include <vector>
namespace boost { namespace numeric { namespace ublas {	

class data_frame_col {
public:
    template<typename T>
    using store_type = boost::numeric::ublas::vector<T>;
    data_frame_col() = default;
    template<typename T> 
    data_frame_col(std::string col_name, const std::vector<T>& col_vec) {
        build_by_vec<T>(col_name, col_vec);
    }
    template<typename T>
    void build_by_vec(std::string col_name, const std::vector<T>& other);
    template<typename T> 
    int get_size() {
        if (vals<T>.count(this))
            return vals<T>[this].size();
        else return 0;
    }
    template<typename T>
    T& at(size_t index) {
        // need to handle exception here
        return vals<T>[this][index];
    }
    template<typename T>
    const T& at(size_t index) const {
        // need to handle exception here
        return vals<T>[this][index];
    }
    data_frame_col(const data_frame_col& _other) {
        *this = _other;
    }
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
    template<typename T>
    store_type<T>& get_vector() {
        return vals<T>[this];
    }
    template<typename T>
    const store_type<T>& get_vector() const {
        return vals<T>[this];
    }
    std::string col_name;
    template<class F>
    void visit(F&& f, int index, const std::string& col_name) {
        visit_impl(f, index, col_name, typename std::decay_t<F>::types{});
    }
    template<class F>
    void visit_init(F&& f) {
        visit_init_impl(f, typename std::decay_t<F>::types{});
    }
    template<class F>
    void visit_print(F&& f, int index) {
        visit_print_impl(f, index, typename std::decay_t<F>::types{});
    }
    template<class...>
    struct type_list{};
    template<class... TypeLists>
    struct init_base {
        using types = boost::numeric::ublas::data_frame_col::type_list<TypeLists...>;
    };
    template<class... TypeLists>
    struct visitor_base {
        using types = boost::numeric::ublas::data_frame_col::type_list<TypeLists...>;
    };
private:
    void clear() {
        for (auto&& clear_func : clear_functions) {
            clear_func(*this);
        }
    }
    template<class F, template<class...> class Typelists, class... Types>
    void visit_impl(F&& f, int index, const std::string& col_name, Typelists<Types...>) {
        (..., visit_impl_help<std::decay_t<F>, Types>(f, index, col_name));
    }
    template<class F, template<class...> class Typelists, class... Types>
    void visit_init_impl(F&& f, Typelists<Types...>) {
        (..., visit_init_impl_help<std::decay_t<F>, Types>(f));
    }
    template<class F, template<class...> class Typelists, class... Types>
    void visit_print_impl(F&& f, int index,Typelists<Types...>) {
        (..., visit_print_impl_help<std::decay_t<F>, Types>(f, index));
    }
    template<class T, class U>
    void visit_impl_help(T& visitor, int index, const std::string& col_name) {
        if (vals<U>[this].size() > 0) 
            visitor(at<U>(index), index, col_name);        
    }
    template<class T, class U>
    void visit_init_impl_help(T& visitor) {
        if (vals<U>[this].size() > 0) 
            visitor(vals<U>[this][0]);
    }
    template<class T, class U>
    void visit_print_impl_help(T& visitor, int index) {
        if (vals<U>[this].size() > 0) 
            visitor(vals<U>[this][index]);
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