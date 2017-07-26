#ifndef SNACKIS_DB_REC_HPP
#define SNACKIS_DB_REC_HPP

#include <map>
#include <string>

#include "snackis/core/opt.hpp"
#include "snackis/core/val.hpp"

namespace snackis {
namespace db {
  template <typename RecT>
  struct BasicCol;

  template <typename RecT, typename ValT>
  struct Col;

  template <typename RecT>
  struct Schema;

  template <typename RecT>
  struct Rec: std::map<const BasicCol<RecT> *, Val> {
    Rec();
    Rec(const Schema<RecT> &scm, const RecT &src);
    Rec(const Schema<RecT> &scm, const Rec<RecT> &src);
  };

  template <typename RecT>
  Rec<RecT>::Rec()
  { }

  template <typename RecT>
  Rec<RecT>::Rec(const Schema<RecT> &scm, const RecT &src) {
    copy(scm, *this, src);
  }

  template <typename RecT>
  Rec<RecT>::Rec(const Schema<RecT> &scm, const Rec<RecT> &src) {
    copy(scm, *this, src);
  }

  template <typename RecT, typename ValT>
  opt<ValT> get(const Rec<RecT> &rec, const Col<RecT, ValT> &col) {
    auto found(rec.find(&col));
    return (found == rec.end()) ? nullopt : opt<ValT>(get<ValT>(found->second));
  }

  template <typename RecT, typename ValT>
  void set(Rec<RecT> &rec, const Col<RecT, ValT> &col, const ValT &val) {
    rec[&col] = val;
  }

  template <typename RecT>
  void copy(RecT &dest, const db::Rec<RecT> &src) {
    for (auto &f: src) { f.first->set(dest, f.second); }
  }
}}

#endif
