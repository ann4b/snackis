#ifndef SNACKIS_DB_TABLE_HPP
#define SNACKIS_DB_TABLE_HPP

#include <cstdint>
#include <set>

#include "snackis/core/data.hpp"
#include "snackis/core/fmt.hpp"
#include "snackis/core/func.hpp"
#include "snackis/core/int64_type.hpp"
#include "snackis/core/opt.hpp"
#include "snackis/core/str_type.hpp"
#include "snackis/core/type.hpp"
#include "snackis/core/stream.hpp"
#include "snackis/crypt/secret.hpp"
#include "snackis/db/change.hpp"
#include "snackis/db/ctx.hpp"
#include "snackis/db/error.hpp"
#include "snackis/db/index.hpp"
#include "snackis/db/rec.hpp"
#include "snackis/db/trans.hpp"

namespace snackis {  
namespace db {
  template <typename RecT, typename...KeyT>
  struct Table;
  
  template <typename RecT, typename...KeyT>
  struct Table: Index<RecT> {
    using Key = db::Key<RecT, KeyT...>;
    using Cols = std::initializer_list<const BasicCol<RecT> *>;
    using OnInsert = func<void (Rec<RecT> &)>;
    using OnUpdate = func<void (const Rec<RecT> &, Rec<RecT> &)>;
    
    const Key key;
    std::set<Index<RecT> *> indexes;
    std::map<typename Key::Type, Rec<RecT>> recs;
    std::vector<OnInsert> on_insert;
    std::vector<OnUpdate> on_update;
    
    Table(Ctx &ctx,
	  const str &name,
	  const Key &key,
	  const Schema<RecT> &cols);
    virtual ~Table();

    bool insert(const Rec<RecT> &rec) override;
    bool update(const Rec<RecT> &rec) override;
    bool erase(const Rec<RecT> &rec) override;

    void dump(std::ostream &out) override;
    void slurp() override;
  };
    
  enum TableOp {TABLE_INSERT, TABLE_UPDATE, TABLE_ERASE};

  template <typename RecT, typename...KeyT>
  struct TableChange: Change {
    TableOp op;
    Table<RecT, KeyT...> &table;
    const Rec<RecT> rec;

    TableChange(TableOp op, Table<RecT, KeyT...> &table, const Rec<RecT> &rec);
    Path table_path() const override;
    void write(std::ostream &out) const override;
  };

  template <typename RecT, typename...KeyT>
  struct Insert: TableChange<RecT, KeyT...> {
    Insert(Table<RecT, KeyT...> &table, const Rec<RecT> &rec);    
    void apply(Ctx &ctx) const override;
    void rollback() const override;
    void undo() const override;
  };

  template <typename RecT, typename...KeyT>
  struct Update: TableChange<RecT, KeyT...> {
    const Rec<RecT> prev_rec;
    Update(Table<RecT, KeyT...> &table,
	   const Rec<RecT> &rec,
	   const Rec<RecT> &prev_rec);    
    void apply(Ctx &ctx) const override;
    void rollback() const override;
    void undo() const override;
  };

  template <typename RecT, typename...KeyT>
  struct Erase: TableChange<RecT, KeyT...> {
    Erase(Table<RecT, KeyT...> &table, const Rec<RecT> &rec);    
    void apply(Ctx &ctx) const override;
    void rollback() const override;
    void undo() const override;
  };

  template <typename RecT, typename...KeyT>
  opt<RecT> load(Table<RecT, KeyT...> &tbl, RecT &rec) {
    auto k(tbl.key(rec));
    auto fnd(tbl.recs.find(k));
    if (fnd == tbl.recs.end()) { return nullopt; }
    copy(tbl, rec, fnd->second);
    return rec;
  }

  template <typename RecT, typename...KeyT>
  opt<Rec<RecT>> load(Table<RecT, KeyT...> &tbl, Rec<RecT> &rec) {
    auto k(tbl.key(rec));
    auto fnd(tbl.recs.find(k));
    if (fnd == tbl.recs.end()) { return nullopt; }
    copy(tbl, rec, fnd->second);
    return rec;
  }

  template <typename RecT, typename...KeyT>
  const Rec<RecT> *find(Table<RecT, KeyT...> &tbl,
			const typename Key<RecT, KeyT...>::Type &key) {
    auto fnd(tbl.recs.find(key));
    if (fnd == tbl.recs.end()) { return nullptr; }
    return &fnd->second;
  }

  template <typename RecT, typename...KeyT>
  const Rec<RecT> *find(Table<RecT, KeyT...> &tbl, const KeyT &...vals) {
    return find(tbl, tbl.key(vals...));
  }

  template <typename RecT, typename...KeyT>
  const Rec<RecT> *find(Table<RecT, KeyT...> &tbl, const Rec<RecT> &rec) {
    return find(tbl, tbl.key(rec));
  }

  template <typename RecT, typename...KeyT>
  const Rec<RecT> &get(Table<RecT, KeyT...> &tbl,
		       const typename Key<RecT, KeyT...>::Type &key) {
    auto fnd(tbl.recs.find(key));
    CHECK(fnd != tbl.recs.end(), _);
    return fnd->second;
  }

  template <typename RecT, typename...KeyT>
  const Rec<RecT> &get(Table<RecT, KeyT...> &tbl, const KeyT &...vals) {
    return get(tbl, tbl.key(vals...));
  }

  template <typename RecT, typename...KeyT>
  const Rec<RecT> &get(Table<RecT, KeyT...> &tbl, const Rec<RecT> &rec) {
    return get(tbl, tbl.key(rec));
  }

  template <typename RecT, typename...KeyT>
  bool insert(Table<RecT, KeyT...> &tbl, const RecT &rec) {
    Rec<RecT> trec;
    copy(tbl, trec, rec);
    return insert(tbl, trec);
  }

  template <typename RecT, typename...KeyT>
  bool insert(Table<RecT, KeyT...> &tbl, const Rec<RecT> &rec) {
    TRACE(fmt("Inserting into table: %0", tbl.name));
    auto k(tbl.key(rec));
    auto fnd(tbl.recs.find(k));
    if (fnd != tbl.recs.end()) { return false; }

    for (auto idx: tbl.indexes) {
      Rec<RecT> irec;
      copy(*idx, irec, rec);
      if (!irec.empty()) { insert(*idx, irec); }
    }

    auto it(tbl.recs.emplace(k, db::Rec<RecT>()).first);
    copy(tbl, it->second, rec);
    for (auto e: tbl.on_insert) { e(it->second); }
    log_change(get_trans(tbl.ctx), new Insert<RecT, KeyT...>(tbl, it->second));
    return true;
  }

  template <typename RecT, typename...KeyT>
  bool update(Table<RecT, KeyT...> &tbl, const RecT &rec) {
    Rec<RecT> trec;
    copy(tbl, trec, rec);
    return update(tbl, trec);
  }

  template <typename RecT, typename...KeyT>
  bool update(Table<RecT, KeyT...> &tbl, const Rec<RecT> &rec) {
    TRACE(fmt("Updating table: %0", tbl.name));
    auto k(tbl.key(rec));
    auto fnd(tbl.recs.find(k));
    if (fnd == tbl.recs.end() || rec == fnd->second) { return false; }
    
    for (auto idx: tbl.indexes) {
      erase(*idx, fnd->second);
      
      Rec<RecT> irec;
      copy(*idx, irec, rec);
      if (!irec.empty()) { insert(*idx, irec); }
    }
    
    auto prev(fnd->second);
    tbl.recs.erase(fnd);
    auto it(tbl.recs.emplace(k, db::Rec<RecT>()).first);
    copy(tbl, it->second, rec);
    for (auto e: tbl.on_update) { e(prev, it->second); }
    log_change(get_trans(tbl.ctx), new Update<RecT, KeyT...>(tbl, it->second, prev));
    return true;
  }

  template <typename RecT, typename...KeyT>
  bool upsert(Table<RecT, KeyT...> &tbl, const RecT &rec) {
    if (insert(tbl, rec)) { return true; }
    update(tbl, rec);
    return false;
  }

  template <typename RecT, typename...KeyT>
  bool erase(Table<RecT, KeyT...> &tbl, const RecT &rec) {
    Rec<RecT> trec;
    copy(tbl.key, trec, rec);
    return erase(tbl, trec);
  }

  template <typename RecT, typename...KeyT>
  bool erase(Table<RecT, KeyT...> &tbl, const Rec<RecT> &rec) {
    TRACE(fmt("Erasing from table: %0", tbl.name));
    auto k(tbl.key(rec));
    auto fnd(tbl.recs.find(k));
    if (fnd == tbl.recs.end()) { return false; }
    log_change(get_trans(tbl.ctx), new Erase<RecT, KeyT...>(tbl, fnd->second));
    for (auto idx: tbl.indexes) { erase(*idx, fnd->second); }
    tbl.recs.erase(fnd);
    return true;
  }

  template <typename RecT, typename...KeyT>
  void write(Table<RecT, KeyT...> &tbl, TableOp _op,
	     const Rec<RecT> &rec,
	     std::ostream &out) {
    uint8_t op(_op);
    out.write(reinterpret_cast<const char *>(&op), sizeof op);
    write(tbl, rec, out, tbl.ctx.secret);
  }

  template <typename RecT, typename...KeyT>
  void dump(Table<RecT, KeyT...> &tbl, std::ostream &out) {    
    for (auto &rec: tbl.recs) {
      write(tbl, TABLE_INSERT, rec.second, out);
    }
  }

  template <typename RecT, typename...KeyT>
  void slurp(Table<RecT, KeyT...> &tbl, std::istream &in) {    
    while (true) {
      uint8_t op;
      in.read(reinterpret_cast<char *>(&op), sizeof op);

      if (in.eof()) {
	in.clear();
	break;
      }

      if (in.fail()) {
	in.clear();
	ERROR(Db, fmt("Failed reading: %0", tbl.name));
      }
            
      Rec<RecT> rec;
      read(tbl, in, rec, tbl.ctx.secret);

      switch (op) {
      case TABLE_INSERT:
	tbl.recs.emplace(tbl.key(rec), rec);
	break;
      case TABLE_UPDATE: {
	auto k(tbl.key(rec));
	tbl.recs.erase(k);
	tbl.recs.emplace(k, rec);
	break;
      }
      case TABLE_ERASE:
	tbl.recs.erase(tbl.key(rec));
	break;
      default:
	log(tbl.ctx, fmt("Invalid table operation: %0", op));
      }
    }
  }

  template <typename RecT, typename...KeyT>
  void slurp(Table<RecT, KeyT...> &tbl) {
    std::fstream f;
    
    f.open(get_path(tbl.ctx, tbl.name + ".tbl").string(),
	      std::ios::in | std::ios::binary);
    if (f.fail()) {
      ERROR(Db, fmt("Failed opening file: %0", tbl.name));
      return;
    }

    slurp(tbl, f);
    f.close();
  }

  template <typename RecT, typename...KeyT>
  void copy(Table<RecT, KeyT...> &dest, const Table<RecT, KeyT...> &src) {
    std::copy(src.recs.begin(), src.recs.end(),
	      std::inserter(dest.recs, dest.recs.end()));
  }
  
  template <typename RecT, typename...KeyT>
  Table<RecT, KeyT...>::Table(Ctx &ctx,
			      const str &name,
			      const Key &key,
			      const Schema<RecT> &cols):
    Index<RecT>(ctx, name, cols),
    key(key)
  {
    for_each(key, [this](auto c) { add(*this, *c); });
    ctx.tables.emplace(name, this);
  }

  template <typename RecT, typename...KeyT>
  Table<RecT, KeyT...>::~Table() {
    this->ctx.tables.erase(this->name);
  }

  template <typename RecT, typename...KeyT>
  bool Table<RecT, KeyT...>::insert(const Rec<RecT> &rec) {
    return db::insert(*this, rec);
  }
  
  template <typename RecT, typename...KeyT>
  bool Table<RecT, KeyT...>::update(const Rec<RecT> &rec) {
    return db::update(*this, rec);
  }
  
  template <typename RecT, typename...KeyT>
  bool Table<RecT, KeyT...>::erase(const Rec<RecT> &rec) {
    return db::erase(*this, rec);
  }
  
  template <typename RecT, typename...KeyT>
  void Table<RecT, KeyT...>::dump(std::ostream &out) { db::dump(*this, out); }

  template <typename RecT, typename...KeyT>
  void Table<RecT, KeyT...>::slurp() { db::slurp(*this); }

  template <typename RecT, typename...KeyT>
  TableChange<RecT, KeyT...>::TableChange(TableOp op,
					  Table<RecT, KeyT...> &table,
					  const Rec<RecT> &rec):
    op(op), table(table), rec(rec)
  { }

  template <typename RecT, typename...KeyT>
  Path TableChange<RecT, KeyT...>::table_path() const {
    return table.path;
  }

  template <typename RecT, typename...KeyT>
  void TableChange<RecT, KeyT...>::write(std::ostream &out) const {
    db::write(this->table, this->op, this->rec, out);
  }

  template <typename RecT, typename...KeyT>
  Insert<RecT, KeyT...>::Insert(Table<RecT, KeyT...> &table, const Rec<RecT> &rec):
    TableChange<RecT, KeyT...>(TABLE_INSERT, table, rec)
  { }
  
  template <typename RecT, typename...KeyT>
  void Insert<RecT, KeyT...>::apply(Ctx &ctx) const {
    auto &tbl(get_table<RecT, KeyT...>(ctx, this->table.name));
    tbl.recs.emplace(tbl.key(this->rec), this->rec);
  }

  template <typename RecT, typename...KeyT>
  void Insert<RecT, KeyT...>::rollback() const {
    this->table.recs.erase(this->table.key(this->rec));
  }

  template <typename RecT, typename...KeyT>
  void Insert<RecT, KeyT...>::undo() const {
    erase(this->table, this->rec);
  }

  template <typename RecT, typename...KeyT>
  Update<RecT, KeyT...>::Update(Table<RecT, KeyT...> &table,
				const Rec<RecT> &rec,
				const Rec<RecT> &prev_rec):
    TableChange<RecT, KeyT...>(TABLE_UPDATE, table, rec),
    prev_rec(prev_rec)
  { }

  template <typename RecT, typename...KeyT>
  void Update<RecT, KeyT...>::apply(Ctx &ctx) const {
    auto &tbl(get_table<RecT, KeyT...>(ctx, this->table.name));
    auto k(tbl.key(this->rec));
    tbl.recs.erase(k);
    tbl.recs.emplace(k, this->rec);
  }

  template <typename RecT, typename...KeyT>
  void Update<RecT, KeyT...>::rollback() const {
    auto k(this->table.key(this->prev_rec));
    this->table.recs.erase(k);
    this->table.recs.emplace(k, this->prev_rec);
  }

  template <typename RecT, typename...KeyT>
  void Update<RecT, KeyT...>::undo() const {
    update(this->table, this->prev_rec);
  }

  template <typename RecT, typename...KeyT>
  Erase<RecT, KeyT...>::Erase(Table<RecT, KeyT...> &table, const Rec<RecT> &rec):
    TableChange<RecT, KeyT...>(TABLE_ERASE, table, rec)
  { }

  template <typename RecT, typename...KeyT>
  void Erase<RecT, KeyT...>::apply(Ctx &ctx) const {
    auto &tbl(get_table<RecT, KeyT...>(ctx, this->table.name));
    tbl.recs.erase(tbl.key(this->rec));
  }

  template <typename RecT, typename...KeyT>
  void Erase<RecT, KeyT...>::rollback() const {
    this->table.recs.emplace(this->table.key(this->rec), this->rec);
  }

  template <typename RecT, typename...KeyT>
  void Erase<RecT, KeyT...>::undo() const {
    insert(this->table, this->rec);
  }
}}

#endif
