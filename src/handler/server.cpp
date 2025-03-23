#include "server.h"
#include "constant/status.h"
#include "strategy/strategy.h"
namespace suggest {
base::Status SuggestServer::Init(Context *ctx) { return base::Status::OK; }
base::Status SuggestServer::Exec(Context *ctx) { 
    RecallProcessor recallProcessor;
    recallProcessor.Exec(ctx);    
    return base::Status::OK; 
}
} // namespace suggest