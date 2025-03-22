#pragma once
#include "baseStrategy.h"
namespace suggest {
class Context {
    public:
    Context(const HttpRequest* req, HttpResponse *rep): req_(req), resp_(rep){}
    const HttpRequest* req_;
    HttpResponse * resp_;

};
} // namespace suggest