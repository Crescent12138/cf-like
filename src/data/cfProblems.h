#pragma once
#include "proto/feed.pb.h"
#include "com/dataType.h"
#include "client/cfClient.h"
namespace suggest {
    class CfProblems : public DictData<std::string, Feed>{
        public:
        CfProblems(){
            CfClient client{};
            // parse TODO
        }
    };
    class CfProblemHandler {
        public:
        static void load(){
            std::shared_ptr<CfProblems> pk_ptr = std::make_shared<CfProblems>();
            list_ptr = pk_ptr;
        }
        static std::shared_ptr<CfProblems> list_ptr ;
        
    };
    static CfProblemHandler cfProblemHandler;
    std::shared_ptr<CfProblems> CfProblemHandler::list_ptr = nullptr;
}