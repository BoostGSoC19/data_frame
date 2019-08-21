# data_frame
Proposed data_frame for boost ublas
This GSOC project aims at building a data frame API based on Boost ublas, inspired from R, Python and many other data frames implementations. The idea is to build a list of column data which can support indexing and slicing and some relational computation. 
To create a `data_frame`, you have to provide the types it can collect. Within those types, you can add any number of columns. 
```
using type_collection = type_list<int, std::string>::types;  
std::vector<int> int_vec = {0, 1, 2, 3, 4};
std::vector<std::string> str_vec = {"test0", "test1", "test2", "test3", "test4"};
data_frame df(type_collection{});
df.add_column("int_vec", int_vec);
df.add_column("str_vec", str_vec);
```
It's also possible to construct `data_frame` with `std::tuple`. 
```
using namespace std::string_literals;
auto df1 = make_from_tuples(std::vector{std::make_tuple(1, 3.3, "hello"s), 
                                        std::make_tuple(2, 2.2, "world"s), 
                                        std::make_tuple(3, 1.1, "github"s)}, 
                                        {"int_vec", "double_vec", "str_vec"});
auto* df2 = make_from_tuples({  {0, 3.4, "hello"s}, 
                                {2, 2.2, "world"s}, 
                                {3, 1.1, "github"s}   }, 
                                {"int_vec", "double_vec", "str_vec"}, 
                                std::tuple<int, double, std::string>{});
```
For `data_frame`, it can support multiple relational operations or set operations. The return value is a view of original `data_frame`. `data_frame_view` only contains column index and any operation on `data_frame_view` will return another `data_frame_view`. You can chain different operations on the same `data_frame_view`. 
### select 
```
using type_collection = type_list<double, long>::types;
data_frame df = type_collection{};
df.from_tuples(std::vector{ std::make_tuple(3.3, 10L), 
                            std::make_tuple(2.2, 40L), 
                            std::make_tuple(1.1, 50L)}, 
                            {"double_vec", "long_vec"});
auto cur_view = df.select<long>("long_vec", [](long curVal) {
                        return curVal >= 40;
                    }).apply_with_index({0, 1, 2}, [](auto& t) {
                        return t * 2;
                    });
```
### join
```
using type_collection1 = type_list<double, long>::types;
using type_collection2 = type_list<std::string, int, double>::types;
data_frame df1 = type_collection1{};
data_frame df2 = type_collection2{};
df1.from_tuples(std::vector{std::make_tuple(3.3, 10L), 
                            std::make_tuple(2.2, 40L)}, 
                            {"double_vec", "long_vec"});
df2.from_tuples(std::vector{std::make_tuple(3.3, "hello"s, 10), 
                            std::make_tuple(3.3, "hello"s, 10),
                            std::make_tuple(2.2, "world"s, 40),
                            std::make_tuple(2.2, "world"s, 40),  
                            std::make_tuple(1.1, "test"s, 50)}, 
                            {"double_vec", "str_vec", "int_vec"});
auto df3 = combine_inner<double>(df1, df2, 
                                "double_vec", 
                                std::tuple<double, long>{}, 
                                {"double_vec", "long_vec"},
                                std::tuple<double, std::string, int>{}, 
                                {"double_vec", "str_vec", "int_vec"});
```
### row set intersect
```
using type_collection = type_list<std::string, int, double>::types;
data_frame df1 = type_collection{};
df1.from_tuples(std::vector{std::make_tuple(3.3, "hello"s, 10), 
                            std::make_tuple(2.2, "world"s, 40),
                            std::make_tuple(7.2, "test"s, 70)}, 
                            {"double_vec", "str_vec", "int_vec"});
data_frame df2 = type_collection{};
df2.from_tuples(std::vector{std::make_tuple(3.3, "hello"s, 10), 
                            std::make_tuple(3.3, "hello"s, 10),
                            std::make_tuple(2.2, "world"s, 40),
                            std::make_tuple(2.2, "world"s, 40),  
                            std::make_tuple(1.1, "github"s, 50)}, 
                            {"double_vec", "str_vec", "int_vec"});
auto df3 = intersect(df1, df2, 
                    std::tuple<double, std::string, int>{}, 
                    {"double_vec", "str_vec", "int_vec"});
```