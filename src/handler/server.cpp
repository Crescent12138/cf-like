#include "server.h"
#include "constant/status.h"
namespace suggest {
base::Status SuggestServer::Init(Context *ctx) { return base::Status::OK; }
base::Status SuggestServer::Exec(Context *ctx) { return base::Status::OK; }
} // namespace suggest