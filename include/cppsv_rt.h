#ifndef CPPSV_INCLUDE_CPPSV_RT_H
#define CPPSV_INCLUDE_CPPSV_RT_H

#include <cstddef>
#include <cstdint>
#include <utility>
#include <string>
#include <vector>

#include "cppsv_common.h"
#include "convert.h"

namespace cppsv {
    template <typename CharT>
    class runtime_cppsv_view {
    public:
        using view_type = std::basic_string_view<CharT>;
        using value_type = CharT;
    private:
        // Calculate column count (defined by the first row)
        static size_t calc_x(const auto& data) noexcept {
            // At least 1 column
            size_t out = 1;
            for (bool in_quotes = false; auto chr : data) {
                in_quotes ^= chr == '"';
                if (!in_quotes) {
                    if (chr == ',') ++out;
                    if (chr == '\n') break;
                }
            }
            return out;
        }

        // Calculate row count
        static size_t calc_y(const auto& data, size_t x) noexcept {
            size_t out = 1;
            size_t index = 0;
            for (bool in_quotes = false; auto chr : data) {
                in_quotes ^= chr == '"';
                if (!in_quotes) {
                    if (chr == ',' && index < x)
                        ++out, ++index;
                    if (chr == '\n')
                        ++out, index = 0;
                }
            }
            return out / x;
        }

        // Strip wrapping quotes, comma
        static view_type strip_field(view_type view) noexcept {
            if (!view.empty() && (view.front() == ','))
                view.remove_prefix(1);
            if (view.length() > 1 && view.front() == '"' && view.back() == '"') {
                view.remove_prefix(1);
                view.remove_suffix(1);
            }
            return view;
        }
        
        // A 2D vector of string views of each field in the csv
        // Is not exposed - it can be iterated over, but individual entries are never returned
        static auto calc_fields(const std::basic_string<CharT>& data) noexcept {
            if (!cppsv_header<CharT>::has_header(data))
                return std::vector<std::vector<view_type>>();
            auto data_view = view_type(data);
            data_view.remove_prefix(cppsv_header<CharT>::size);
            size_t x = calc_x(data_view);
            size_t y = calc_y(data_view, x);
            auto out = std::vector<std::vector<view_type>>(y, std::vector<view_type>(x));
            auto first = data_view.begin();
            auto last = data_view.end();
            auto field_first = first;
            size_t index_x = 0;
            size_t index_y = 0;
            for (bool in_quotes = false; first != last; ++first) {
                auto chr = *first;
                in_quotes ^= chr == '"';
                if (!in_quotes) {
                    if ((chr == ',' || chr == '\n') && index_x < x) {
                        out[index_y][index_x++] = strip_field({ field_first, first });
                        field_first = first != last ? first + 1 : first;
                    }
                    if (chr == '\n') {
                        index_x = 0;
                        ++index_y;
                    }
                }
            }
            return out;
        }

        std::basic_string<CharT> data;
        std::vector<std::vector<view_type>> fields; 
    public:
        explicit runtime_cppsv_view(std::basic_string<CharT> data) noexcept
            : data(data), fields(calc_fields(this->data)) {}
        
        explicit runtime_cppsv_view(std::basic_string<CharT>&& data) noexcept
            : data(std::move(data)), fields(calc_fields(this->data)) {}

        // Get the column count in the csv
        // The column count is defined by the number of fields in the first row
        size_t columns() const noexcept {
            return this->fields[0].size();
        }

        // Get the row count in the csv
        size_t rows() const noexcept {
            return this->fields.size();
        }

        // Get a csv row by the row index as a vector of fields
        const auto& get_row(size_t row_index) const noexcept {
            return this->fields.at(row_index);
        }

        // Get a csv field by the column and row indices
        template <size_t IColumn, size_t IRow>
        const auto& get_field(size_t column_index, size_t row_index) const noexcept {
            return this->fields.at(row_index).at(column_index);
        }

        // Get a csv field by the column name and row index
        const auto& get_field(const auto& column_name, size_t row_index) const noexcept {
            return this->get_field(this->get_row(row_index), column_name);
        }

        // Get a field from a csv row by column index
        static const auto& get_field(const std::vector<view_type>& row, size_t column_index) noexcept {
            return row.at(column_index);
        }

        // Get a field from a tuple-like csv row by column name
        const auto& get_field(const std::vector<view_type>& row, const auto& column_name) const noexcept {
            size_t index = 0;
            for (const auto& field : this->fields[0]) {
                if (field == column_name) break;
                ++index;
            }
            return row.at(index);
        }

        // Iterate over all fields,
        // calling "function(std::basic_string_view<value_type>)"
        // Accepts only constant evaluated functions
        void for_each_field(auto function) const noexcept {
            for (const auto& row : fields)
                for (const auto& field : row)
                    function(field);
        }

        // Iterate over all rows,
        // calling "function(std::vector<std::basic_string_view<value_type>>)"
        // Accepts only constant evaluated functions
        void for_each_row(auto function) const noexcept {
            for (const auto& row : fields)
                function(row);
        }

        // Iterate over fields
        // while "function(std::basic_string_view<value_type>)" evaluates to "true"
        auto find_field(auto function) const noexcept {
            for (const auto& row : fields)
                for (const auto& field : row)
                    if (function(field)) return field;
            return view_type{};
        }

        // Iterate over all rows
        // while "function(std::vector<std::basic_string_view<value_type>>)" evaluates to "true"
        auto find_row(auto function) const noexcept {
            for (const auto& row : fields) 
                if (function(row)) return row;
            return std::vector<view_type>{ this->columns() };
        }
    };
} 

#endif /* CPPSV_INCLUDE_CPPSV_RT_H */