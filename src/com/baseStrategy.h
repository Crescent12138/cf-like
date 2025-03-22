#pragma once
#include <algorithm>
#include <iostream>
#include "constant/status.h"
#include "proto/http.pb.h"
namespace suggest {
class Context;
class BaseStrategy {
public:
  base::Status Exec(Context *ctx){return base::Status::OK;};
  base::Status PreExec(Context *ctx){return base::Status::OK;};
  base::Status AfterExec(Context *ctx){return base::Status::OK;};
};
} // namespace suggest