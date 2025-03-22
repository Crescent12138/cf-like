#pragma once
#include "baseStrategy.h"
namespace suggest {
class Context {
    public:
    Context(const HttpRequest* req, HttpResponse *rep): req_(req), rep_(rep){}
    const HttpRequest* req_;
    HttpResponse * rep_;

};
} // namespace suggest