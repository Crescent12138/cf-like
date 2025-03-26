#pragma once
#include <butil/containers/flat_map.h>

namespace suggest {
    template<class K, class V>
class DictData {
public:
    DictData(){};
    V find(const K& item){
        return map[item];
    }
    
protected:
    butil::FlatMap<K, V> map;
};
}  // namespace suggest