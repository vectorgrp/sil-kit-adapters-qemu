
#include <fmt/format.h>

#include <ib/version.hpp>

int main()
{
    fmt::print("IntegrationBus Version: {}\n", ib::version::String());
}
