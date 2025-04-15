#pragma once
#include <brpc/channel.h>
#include <butil/logging.h>
#include <google/protobuf/util/json_util.h>
#include <json2pb/rapidjson.h>

// problem数据校验
#define VALIDATE_PROBLEM_FIELD(problem, field, check_type, index)                \
    do {                                                                         \
        if (!(problem).HasMember(field) || !(problem)[field].check_type()) {     \
            LOG(WARNING) << "Invalid " #field " in problem at index: " << index; \
            continue;                                                            \
        }                                                                        \
    } while (0)

// json 数据校验
#define VALIDATE_JSON_FIELD(doc, field, type, value, notify)       \
    do {                                                           \
        if (!(doc).HasMember(field) || !(doc)[field].Is##type()) { \
            if (notify) {                                          \
                LOG(WARNING) << "Invalid " << field;               \
            }                                                      \
                                                                   \
            return false;                                          \
        }                                                          \
        value = (doc)[field].Get##type();                          \
    } while (0)

#define CHECK_JSON_FIELD(doc, field, type) (!(doc).HasMember(field) || !(doc)[field].Is##type())

namespace utils {
static void pb_2_json(const google::protobuf::Message& message, std::string& json) {
    google::protobuf::util::JsonPrintOptions options;
    options.add_whitespace                = true;
    options.always_print_primitive_fields = true;
    options.preserve_proto_field_names    = true;
    google::protobuf::util::MessageToJsonString(message, &json, options);
}

// 数据获取参考型
// 为了防止其他地方引用到，直接先用函数包住
static bool get_rapidjon_string(const butil::rapidjson::Document& js_in, char* key, std::string& value,
                                bool notify = true) {
    VALIDATE_JSON_FIELD(js_in, key, String, value, notify);
    return true;
}

static bool get_rapidjon_int(const butil::rapidjson::Document& js_in, char* key, int& value, bool notify = true) {
    VALIDATE_JSON_FIELD(js_in, key, Int, value, notify);
    return true;
}

static bool get_rapidjon_string(const butil::rapidjson::Value& js_in, char* key, std::string& value,
                                bool notify = true) {
    VALIDATE_JSON_FIELD(js_in, key, String, value, notify);
    return true;
}

static bool get_rapidjon_int(const butil::rapidjson::Value& js_in, char* key, int& value, bool notify = true) {
    VALIDATE_JSON_FIELD(js_in, key, Int, value, notify);
    return true;
}

static bool get_rapidjon_vector_string(const butil::rapidjson::Value& js_in, char* key,
                                       std::vector<std::string>& value) {

    if (js_in.HasMember(key)) {
        auto& ori_tags = js_in[key];
        value.reserve(ori_tags.Size());
        for (int j = 0; j < ori_tags.Size(); j++) {
            value.emplace_back(ori_tags[j].GetString());
        }
        return true;
    }
    return false;
}


}  // namespace utils