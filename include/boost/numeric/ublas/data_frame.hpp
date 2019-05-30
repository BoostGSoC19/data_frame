#ifndef _BOOST_UBLAS_DATA_FRAME_
#define _BOOST_UBLAS_DATA_FRAME_
#include <vector>
#include <string>
#include <memory>
#include <map>

namespace boost { namespace numeric { namespace ublas {
    /**
     */
    class col_base {
    public:
        virtual ~col_base() = 0;
    };
    template<typename T> 
    class col: public col_base {
    public:
        typedef T Type;
        col() = default;
        explicit col(pair<std::string, std::vector<T>> _p): col_name(_p.first), _store(_p.second) {}
        explicit col(const vector<T>& other) : _store(other) {}
        explicit col& operator = (pair<string, std::vector<T>> _p) {
            return col(_p);
        }
        void set_name(string name) { col_name = name; }
    private:
        vector<T> _store;
        string col_name;
    };
    class data_frame {
    public:
        data_frame() = default;
        template<typename T>
        void add_cols(std::string col_name, col<T> col_vector) {

        }
    private:
        std::map<std::string, std::unique_ptr<col_base>> _m;
    };
}}}

#endif