#include "cfClient.h"
namespace suggest{
    brpc::ChannelOptions CfClient::options;
    brpc::Channel        CfClient::codeforce_channel;
}