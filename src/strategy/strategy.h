#pragma once
#include "com/baseStrategy.h"
#include "constant/status.h"

namespace suggest{
    class RecallProcessor :public BaseStrategy{
        public:
        base::Status Exec(Context *ctx) override;
    };
    class MetricProcessor :public BaseStrategy{
        public:
        base::Status Exec(Context *ctx) override;
    };
}