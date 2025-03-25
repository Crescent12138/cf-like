#pragma once
#include <butil/containers/flat_map.h>

namespace suggest {
class DictData {
public:
    DictData(){};
    
protected:
    butil::FlatMap<int, std::string> map;
};
}  // namespace suggest