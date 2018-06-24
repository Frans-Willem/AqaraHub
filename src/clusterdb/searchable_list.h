#ifndef _CLUSTERDB_SEARCHABLE_LIST_H_
#define _CLUSTERDB_SEARCHABLE_LIST_H_
#include <boost/format.hpp>
#include <boost/optional.hpp>
#include <list>
#include <map>
#include "logging.h"

namespace clusterdb {
template <typename T>
class SearchableList {
 public:
  typedef T Item;
  typedef decltype(T::id) Id;
  typedef decltype(T::name) Name;

  bool Add(Item item) {
    if (by_id_.count(item.id) > 0) {
      LOG("SearchableList", warning)
          << "Duplicate ID "
          << boost::str(boost::format("0x%X") % (unsigned int)item.id);
      return false;
    }
    if (by_name_.count(item.name) > 0) {
      LOG("SearchableList", warning) << "Duplicate name '" << item.name << "'";
      return false;
    }
    items_.emplace_back(std::move(item));
    Item& ref = items_.back();
    by_id_[ref.id] = &ref;
    by_name_[ref.name] = &ref;
    return true;
  }

  boost::optional<const Item&> FindByName(const Name& name) const {
    auto found = by_name_.find(name);
    if (found == by_name_.end()) {
      return boost::none;
    }
    return *found->second;
  }

  boost::optional<const Item&> FindById(const Id& id) const {
    auto found = by_id_.find(id);
    if (found == by_id_.end()) {
      return boost::none;
    }
    return *found->second;
  }

 private:
  std::list<Item> items_;
  std::map<Id, Item*> by_id_;
  std::map<Name, Item*> by_name_;
};
}  // namespace clusterdb
#endif  // _CLUSTERDB_SEARCHABLE_LIST_H_
