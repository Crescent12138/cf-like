#include <brpc/channel.h>
#include <butil/logging.h>
#include <json2pb/rapidjson.h>

#include "constant/status.h"

namespace suggest {
class CfClient {
public:
    CfClient() {
        brpc::Controller cntl;
        cntl.http_request().uri() = "https://codeforces.com/api/problemset.problems";  // 设置为待访问的URL
        codeforce_channel.CallMethod(NULL, &cntl, NULL, NULL, NULL /*done*/);
        if (cntl.ErrorCode() != 0) {
            LOG(ERROR) << "error:" << cntl.ErrorCode() << " text:" << cntl.ErrorText() << "\n";
            status = base::HTTP_ERROR;
            return;
        }

        if (doc.Parse(cntl.response_attachment().to_string().c_str()).HasParseError()) {
            LOG(ERROR) << "error: parse codeforces error!!\n";
            status = base::EARLY_RETURN;
            return;
        }
        status = base::OK;
    }

    static base::Status Init() {
        options.protocol           = brpc::PROTOCOL_HTTP;  // or brpc::PROTOCOL_H2
        options.timeout_ms         = 4000;
        options.connect_timeout_ms = 1000;
        if (codeforce_channel.Init("https://codeforces.com" /*any url*/, &options) != 0) {
            LOG(ERROR) << "Fail to initialize codeforce_channel";
            return base::Status::HTTP_ERROR;
        }
        return base::Status::OK;
    }

    base::Status               status = base::UNKNOWN;
    butil::rapidjson::Document doc;

    static brpc::ChannelOptions options;
    static brpc::Channel        codeforce_channel;
};
}  // namespace suggest
