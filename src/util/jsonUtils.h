#pragma once
#include <json2pb/rapidjson.h>

namespace utils {

static int get_rapidjson_string(const butil::rapidjson::Document& js_in, const std::string& key, std::string& value) {

    if (!js_in.HasMember(key) && js_in[key].IsString()) {
        value = js_in[key].GetString();
        return 0;
    } else {
        return -1;
    }
}

static int get_rapidjson_int(const butil::rapidjson::Document& js_in, const std::string& key, int& value) {
    if (!js_in.HasMember(key) && js_in[key].IsInt()) {
        value = js_in[key].GetInt();
        return 0;
    } else {
        return -1;
    }
}
// static int get_rapidjson_Object(const butil::rapidjson::Document& js_in, const std::string& key, int& value) {
//     if (!js_in.HasMember(key) && js_in[key].IsInt()) {
//         value = js_in[key].GetInt();
//         return 0;
//     } else {
//         return -1;
//     }
// }
///< get float number
}  // namespace utils