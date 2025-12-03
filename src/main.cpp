#include <gflags/gflags.h>
#include <butil/logging.h>
#include <brpc/server.h>
#include <brpc/restful.h>
#include <json2pb/pb_to_json.h>
#include "handler/server.h"
#include <brpc/redis.h>
#include <brpc/channel.h>
#include "proto/http.pb.h"
#include "com/baseStrategy.h"
#include "com/context.h"
#include "data/cfProblems.h"
#include "util/timeWheel.h"
#include "util/jsonUtils.h"
DEFINE_int32(port, 8010, "TCP Port of this server");
DEFINE_int32(idle_timeout_s, -1, "Connection will be closed if there is no "
             "read/write operations during the last `idle_timeout_s'");
DEFINE_string(config, "config.json", "parse all param");
DEFINE_string(certificate, "cert.pem", "Certificate file path to enable SSL");
DEFINE_string(private_key, "key.pem", "Private key file path to enable SSL");
DEFINE_string(ciphers, "", "Cipher suite used for SSL connections");

namespace suggest {

// Service with static path.
class HttpServiceImpl : public HttpService {
public:
    HttpServiceImpl() {}
    virtual ~HttpServiceImpl() {}
    void Echo(google::protobuf::RpcController* cntl_base,
              const HttpRequest* req,
              HttpResponse* ,
              google::protobuf::Closure* done) {
        // This object helps you to call done->Run() in RAII style. If you need
        // to process the request asynchronously, pass done_guard.release().
        brpc::ClosureGuard done_guard(done);
        
        brpc::Controller* cntl =
            static_cast<brpc::Controller*>(cntl_base);
        HttpResponse resp;
        butil::IOBufBuilder os;
       
        Context ctx(req, &resp);
        ctx.query = cntl->request_attachment().to_string();

        SuggestServer server;
        server.Init(&ctx);
        server.Exec(&ctx);
        // Fill response.
        cntl->http_response().set_content_type("text/plain");
        // 目前是把 queries 和 body写入返回结果。
        //LOG(INFO) << "Request: " << cntl->request_attachment().to_string();
        std::string json_str;
        utils::pb_2_json(resp, json_str);
        // resp.SerializeToString(&json_str);
        os << json_str;

        os.move_to(cntl->response_attachment());
    }
    void TeamFeed(google::protobuf::RpcController* cntl_base,
        const HttpRequest* req,
        HttpResponse* ,
        google::protobuf::Closure* done){
            brpc::ClosureGuard done_guard(done);
        
            brpc::Controller* cntl =
                static_cast<brpc::Controller*>(cntl_base);
            HttpResponse resp;
            butil::IOBufBuilder os;
           
            Context ctx(req, &resp);
            ctx.query = cntl->request_attachment().to_string();
    
            SuggestServer server;
            server.Init(&ctx);
            server.Exec(&ctx);
            // Fill response.
            cntl->http_response().set_content_type("text/plain");
            // 目前是把 queries 和 body写入返回结果。
            //LOG(INFO) << "Request: " << cntl->request_attachment().to_string();
            std::string json_str;
            utils::pb_2_json(resp, json_str);
            // resp.SerializeToString(&json_str);
            os << json_str;
            os.move_to(cntl->response_attachment());
        }
};

}  // namespace suggest

int main(int argc, char* argv[]) {
    // Parse gflags. We recommend you to use gflags as well.
    GFLAGS_NAMESPACE::ParseCommandLineFlags(&argc, &argv, true);
    suggest::CfClient::Init();
    // init DataDict
    TimeWheel time_wheel;
    time_wheel.initTimeWheel(1000, 1440); // 1秒步长，120分钟(2小时)最大周期
    
    // 立即加载一次数据
    suggest::cfProblemHandler.load();
    
    // 暂时注释掉定时任务，避免时间轮配置问题
    // problemset 更新改为2小时一次，避免频繁请求CF API (2*60*60*1000 = 7200000ms)
    // 必须是steps(1000)的倍数，所以使用7200000
    
    //time_wheel.createTimingEvent(86400000, suggest::cfProblemHandler.forceRefresh);

    //time_wheel.createTimingEvent(300000, suggest::cfProblemHandler.forceRefresh);

    time_wheel.createTimingEvent(7200000, suggest::cfProblemHandler.forceRefresh);

    // Generally you only need one Server.
    brpc::Server server;
    //redis
    {
        brpc::ChannelOptions redis_options;
        redis_options.protocol = brpc::PROTOCOL_REDIS;
        brpc::Channel redis_channel;
          // redis 测试中
        // if (redis_channel.Init("0.0.0.0:6379", &redis_options) != 0) {  // 6379是redis-server的默认端口
        //     LOG(ERROR) << "Fail to init channel to redis-server";
        //     return -1;
        // }
    }

    suggest::HttpServiceImpl http_svc;
    
    // Add services into server. Notice the second parameter, because the
    // service is put on stack, we don't want server to delete it, otherwise
    // use brpc::SERVER_OWNS_SERVICE.
    if (server.AddService(&http_svc,
                          brpc::SERVER_DOESNT_OWN_SERVICE) != 0) {
        LOG(ERROR) << "Fail to add http_svc";
        return -1;
    }

    // Start the server.
    brpc::ServerOptions options;
    options.idle_timeout_sec = FLAGS_idle_timeout_s;
    options.mutable_ssl_options()->default_cert.certificate = FLAGS_certificate;
    options.mutable_ssl_options()->default_cert.private_key = FLAGS_private_key;
    options.mutable_ssl_options()->ciphers = FLAGS_ciphers;
    if (server.Start(FLAGS_port, &options) != 0) {
        LOG(ERROR) << "Fail to start HttpServer";
        return -1;
    }

    // Wait until Ctrl-C is pressed, then Stop() and Join() the server.
    server.RunUntilAskedToQuit();
    return 0;
}