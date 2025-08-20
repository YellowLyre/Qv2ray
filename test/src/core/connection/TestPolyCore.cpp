#include "catch.hpp"
#include "Common.hpp"

#include "core/kernel/PolyCoreKernelManager.hpp"

using namespace PolyCore::core::kernel;

TEST_CASE("PolyCore ParseVersionFromOutput works", "[polycore][version]")
{
    REQUIRE(PolyCoreKernelManager::ParseVersionFromOutput(KernelType::SING_BOX,
            "sing-box version 1.12.0 (go1.21)") == "1.12.0");
    REQUIRE(PolyCoreKernelManager::ParseVersionFromOutput(KernelType::MIHOMO,
            "Mihomo v1.18.5") == "1.18.5");
    REQUIRE(PolyCoreKernelManager::ParseVersionFromOutput(KernelType::MIHOMO,
            "Clash.Meta 2024.02.09") == "2024.02.09");
    REQUIRE(PolyCoreKernelManager::ParseVersionFromOutput(KernelType::XRAY,
            "Xray 1.8.3 (custom)") == "1.8.3");
    REQUIRE(PolyCoreKernelManager::ParseVersionFromOutput(KernelType::V2RAY,
            "V2Ray 5.8.0") == "5.8.0");
    REQUIRE(PolyCoreKernelManager::ParseVersionFromOutput(KernelType::SING_BOX,
            "unexpected\ntext") == "unexpected");
}

TEST_CASE("PolyCore ComputePreferredKernel priority", "[polycore][priority]")
{
    QSet<KernelType> avail;
    avail << KernelType::MIHOMO << KernelType::XRAY;
    REQUIRE(static_cast<int>(PolyCoreKernelManager::ComputePreferredKernel(KernelType::SING_BOX, avail))
            == static_cast<int>(KernelType::MIHOMO));
    avail << KernelType::SING_BOX;
    REQUIRE(static_cast<int>(PolyCoreKernelManager::ComputePreferredKernel(KernelType::SING_BOX, avail))
            == static_cast<int>(KernelType::SING_BOX));
}
