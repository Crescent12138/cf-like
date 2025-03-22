#pragma once
#include <brpc/restful.h>
#include "constant/status.h"
#include "proto/http.pb.h"
#include <brpc/server.h>
#include <json2pb/pb_to_json.h>
#include "com/context.h"
namespace suggest {
class SuggestServer {
    public :
    base::Status Init(Context *ctx);
    base::Status Exec(Context *ctx);
};
} // namespace suggest