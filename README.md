# CPPSV - Compile time .csv parsing with C++
cppsv is a C++20 header only library made for accessing large amounts of data stored in Comma Separated Values (.csv) files at compile time, without the use of other tools or compiler extensions.

The main goals of cppsv are:
1. Provide an immutable compile time construct representing a .csv
2. Adhere to the RFC4180 .csv format specification
3. Restrict the visibility of the data - it should be left behind at compile time, not embedded in the binary
4. Allow comprehension of floating point and integer constants (convert.h header)
5. Portability, no reliance on compiler extensions or build systems
6. No significant compilation time overhead even for large amounts of data

# The cppsv .csv format
cppsv is intrusive to the .csv files it operates on, however, cppsv .csv files are still valid .csv files. The cppsv .csv format requires a header and a footer to be present in the .csv, as such:
```
"\"" R",,,"cppsv-fmt(cppsv"
your,csv,data,here
),,,"cppsv-fmt"
```
The header consists of an opening quote `"` and a raw string literal containing `cppsv"\n`, concatenated together into `"cppsv"\n`, the cppsv magic that is checked before parsing the string literal itself. The raw string literal delimiter is special in the sense that it makes the header conform to RFC4180 - the number of fields should be consistent throughout the .csv and any quote `"` appearing inside a field must be escaped with another quote (like `""`) and only appear in fields that are wrapped in quotes (so `""""`), which is impossible with just a R"()" raw string literal. The amount of commas in the delimiter corresponds to the number of fields per row minus one.`cppsv-fmt` is an optional delimiter string which may be changed or removed (keeping the surrounding quotes). The raw string literal delimiter has a maximum length of 16 characters, which isn't addressed here for simplicity's sake.

NOTE: you may need to split large .csv files into multiple smaller ones to fit your compiler's raw string literal size limit, and include them with CPPSV_VIEW_NEXT.

# Usage
Include `cppsv.h` and `#include` cppsv-formatted (read above) .csv files in a `CPPSV_VIEW` block:
```cpp
#include "cppsv.h"

CPPSV_VIEW_BEGIN
#include "test1.csv"
CPPSV_VIEW_NEXT
#include "test2.csv"
CPPSV_VIEW_NEXT
#include "test3.csv"
CPPSV_VIEW_NAME(testcsv);
```
Iterate over all rows/fields with the corresponding methods like `for_each_row`, `for_each_field`, `find_row`, `find_field` or access specific rows and fields with `get_row` and `get_field`:
```cpp
#include <iostream>

int main() {
    constexpr auto row = testcsv.find_row([](const auto& fields) {
        const auto& [name, age, city, country, email] = fields;
        return !country.compare("Brazil");
    });
    constexpr auto name = testcsv.get_field<"Name">(row);
    constexpr int age = testcsv.get_field<"Age">(row).as<int>();
    // Not interpretable as an integer, compile time error:
    // constexpr auto email = testcsv.get_field<"Email">(row).as<int>();
    int born = 2024 - age;
    std::cout << name.string << " " << born << "\n";
    return 0;
}
```
Most cppsv_view methods are immediate functions (consteval). It is recommended to use lambdas in calls to `for_each_row`, `for_each_field`, `find_row`, `find_field`. The example above can be found in the example folder. Example output:
```
Sofia Oliveira 1989
```