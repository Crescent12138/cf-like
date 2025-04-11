#pragma once
#include <stdlib.h>
#include <cstdlib>
#include <json2pb/rapidjson.h>
#include <google/protobuf/util/json_util.h>
namespace utils {
static void pb_2_json(const google::protobuf::Message& message, std::string& json){
    google::protobuf::util::JsonPrintOptions options;
    options.add_whitespace = true;
    options.always_print_primitive_fields = true;
    options.preserve_proto_field_names = true;
    google::protobuf::util::MessageToJsonString(message, &json, options);
}
// static int get_rapidjson_string(butil::rapidjson::Document& js_in,  std::string key, std::string& value) {

//     if (!js_in.HasMember(key) && js_in[key].IsString()) {
//         value = js_in[key].GetString();
//         return 0;
//     } else {
//         return -1;
//     }
// }

// static int get_rapidjson_int(butil::rapidjson::Document& js_in,  std::string key, int& value) {
//     if (!js_in.HasMember(key) && js_in[key].IsInt()) {
//         value = js_in[key].GetInt();
//         return 0;
//     } else {
//         return -1;
//     }
// }
// static int get_rapidjson_Object(butil::rapidjson::Document& js_in, std::string key, butil::rapidjson::GenericValue<butil::rapidjson::UTF8<> >& value) {
//     if (!js_in.HasMember(key) && js_in[key].IsObject()) {
//         value = js_in[key].GetObejct();
//         return 0;
//     } else {
//         return -1;
//     }
// }
///< get float number
}  // namespace utils