#include "../include/cppsv.h"

CPPSV_VIEW_BEGIN
#include "test1.csv"
CPPSV_VIEW_NEXT
#include "test2.csv"
CPPSV_VIEW_NEXT
#include "test3.csv"
CPPSV_VIEW_NAME(testcsv);

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
