//
// Copyright Aliaksei Levin (levlam@telegram.org), Arseny Smirnov (arseny30@gmail.com) 2014-2018
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include "tl_writer_cpp.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace td {

class TD_TL_writer_jni_cpp : public TD_TL_writer_cpp {
  std::string gen_vector_fetch(std::string field_name, const tl::tl_tree_type *t,
                               const std::vector<tl::var_description> &vars, int parser_type) const;

  std::string gen_vector_store(const std::string &field_name, const tl::tl_tree_type *t,
                               const std::vector<tl::var_description> &vars, int storer_type) const;

  std::string package_name;

  std::string gen_java_field_name(std::string name) const;

  std::string gen_basic_java_class_name(std::string name) const;

  std::string gen_java_class_name(std::string name) const;

  std::string gen_type_signature(const tl::tl_tree_type *tree_type) const;

  std::string get_pretty_field_name(std::string field_name) const override;

  std::string get_pretty_class_name(std::string class_name) const override;

 public:
  TD_TL_writer_jni_cpp(const std::string &tl_name, const std::string &string_type, const std::string &bytes_type,
                       const std::vector<std::string> &ext_include, const std::string &package_name_)
      : TD_TL_writer_cpp(tl_name, string_type, bytes_type, ext_include), package_name(package_name_) {
    for (std::size_t i = 0; i < package_name.size(); i++) {
      if (package_name[i] == '.') {
        package_name[i] = '/';
      }
    }
  }

  bool is_built_in_simple_type(const std::string &name) const override;
  bool is_built_in_complex_type(const std::string &name) const override;

  int get_parser_type(const tl::tl_combinator *t, const std::string &parser_name) const override;
  int get_additional_function_type(const std::string &additional_function_name) const override;
  std::vector<std::string> get_parsers() const override;
  std::vector<std::string> get_storers() const override;
  std::vector<std::string> get_additional_functions() const override;

  std::string gen_base_type_class_name(int arity) const override;
  std::string gen_base_tl_class_name() const override;

  std::string gen_class_begin(const std::string &class_name, const std::string &base_class_name,
                              bool is_proxy) const override;

  std::string gen_field_definition(const std::string &class_name, const std::string &type_name,
                                   const std::string &field_name) const override;

  std::string gen_constructor_id_store(std::int32_t id, int storer_type) const override;

  std::string gen_field_fetch(int field_num, const tl::arg &a, std::vector<tl::var_description> &vars, bool flat,
                              int parser_type) const override;
  std::string gen_field_store(const tl::arg &a, std::vector<tl::var_description> &vars, bool flat,
                              int storer_type) const override;
  std::string gen_type_fetch(const std::string &field_name, const tl::tl_tree_type *tree_type,
                             const std::vector<tl::var_description> &vars, int parser_type) const override;
  std::string gen_type_store(const std::string &field_name, const tl::tl_tree_type *tree_type,
                             const std::vector<tl::var_description> &vars, int storer_type) const override;

  std::string gen_get_id(const std::string &class_name, std::int32_t id, bool is_proxy) const override;

  std::string gen_fetch_function_begin(const std::string &parser_name, const std::string &class_name, int arity,
                                       std::vector<tl::var_description> &vars, int parser_type) const override;
  std::string gen_fetch_function_end(int field_num, const std::vector<tl::var_description> &vars,
                                     int parser_type) const override;

  std::string gen_fetch_function_result_begin(const std::string &parser_name, const std::string &class_name,
                                              const tl::tl_tree *result) const override;
  std::string gen_fetch_function_result_end() const override;
  std::string gen_fetch_function_result_any_begin(const std::string &parser_name, const std::string &class_name,
                                                  bool is_proxy) const override;
  std::string gen_fetch_function_result_any_end(bool is_proxy) const override;

  std::string gen_store_function_begin(const std::string &storer_name, const std::string &class_name, int arity,
                                       std::vector<tl::var_description> &vars, int storer_type) const override;

  std::string gen_fetch_switch_begin() const override;
  std::string gen_fetch_switch_case(const tl::tl_combinator *t, int arity) const override;
  std::string gen_fetch_switch_end() const override;

  std::string gen_additional_function(const std::string &function_name, const tl::tl_combinator *t,
                                      bool is_function) const override;
  std::string gen_additional_proxy_function_begin(const std::string &function_name, const tl::tl_type *type,
                                                  const std::string &class_name, int arity,
                                                  bool is_function) const override;
  std::string gen_additional_proxy_function_case(const std::string &function_name, const tl::tl_type *type,
                                                 const std::string &class_name, int arity) const override;
  std::string gen_additional_proxy_function_case(const std::string &function_name, const tl::tl_type *type,
                                                 const tl::tl_combinator *t, int arity,
                                                 bool is_function) const override;
  std::string gen_additional_proxy_function_end(const std::string &function_name, const tl::tl_type *type,
                                                bool is_function) const override;
};

}  // namespace td
