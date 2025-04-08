#include "constant/status.h"
#include "strategy/strategy.h"
#include <brpc/channel.h>
#include <butil/logging.h>
#include <json2pb/rapidjson.h>
#include "com/context.h"
#include "util/metricManager.h"
namespace suggest {
    
    base::Status MetricProcessor::Exec(Context *ctx){
        
        utils::time << 1 << 2 << 3;
    //    utils::time_minitue.expose();
        // g_request_count.expose("request_count_another");

        // bvar::Dumper dump;
        // g_request_count.dump_exposed(, nullptr);
        return base::Status::OK;
    }
}