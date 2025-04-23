#pragma once
#include "baseStrategy.h"
#include "data/cfProblems.h"
namespace suggest {
class Context {
    public:
    Context(const HttpRequest* req, HttpResponse *rep): req_(req), resp_(rep),cf_problems(CfProblemHandler::list_ptr){}
    const HttpRequest* req_;
    HttpResponse * resp_;
    int rating;
    std::string query;
    std::string name ;
    std::shared_ptr<CfProblems> cf_problems;
};
} // namespace suggest