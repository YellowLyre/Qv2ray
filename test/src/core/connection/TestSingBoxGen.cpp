#include "catch.hpp"
#include "Common.hpp"

#include "core/kernel/PolyCoreKernelInterface.hpp"

using namespace PolyCore::core::kernel;

static CONFIGROOT minimalRoot()
{
    // Build minimal CONFIGROOT with one inbound/outbound
    QJsonObject root;
    // inbound
    QJsonArray inbounds;
    QJsonObject in;
    in["tag"] = "in-socks";
    in["protocol"] = "socks";
    in["listen"] = "127.0.0.1";
    in["port"] = 1080;
    inbounds.push_back(in);
    root["inbounds"] = inbounds;
    // outbound
    QJsonArray outbounds;
    QJsonObject out;
    out["tag"] = "proxy";
    out["protocol"] = "vmess";
    out["settings"] = QJsonObject{ {"vnext", QJsonArray{ QJsonObject{ {"address","example.com"}, {"port",443}, {"users", QJsonArray{ QJsonObject{{"id","aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee"}} } } } } } };
    out["streamSettings"] = QJsonObject{ {"network","ws"} };
    outbounds.push_back(out);
    root["outbounds"] = outbounds;
    return CONFIGROOT(root);
}

TEST_CASE("SingBox: GenerateConfig basic shape", "[singbox][gen]")
{
    SingBoxKernelAdapter adapter;
    auto root = minimalRoot();
    const auto jsonText = adapter.GenerateConfig(root);
    REQUIRE_FALSE(jsonText.isEmpty());

    const auto jsonObj = QJsonDocument::fromJson(jsonText.toUtf8()).object();
    REQUIRE(jsonObj.contains("inbounds"));
    REQUIRE(jsonObj.contains("outbounds"));
    REQUIRE(jsonObj.contains("route"));
    REQUIRE(jsonObj["outbounds"].toArray().size() >= 3); // proxy + direct + block

    // Route details
    const auto routeObj = jsonObj.value("route").toObject();
    REQUIRE(routeObj.contains("final"));
    REQUIRE(routeObj.value("final").toString() == "proxy");
    REQUIRE(routeObj.contains("rules"));
    REQUIRE(routeObj.value("rules").isArray());
    REQUIRE(routeObj.value("rules").toArray().size() >= 1);
    // basic rule shape: ensure objects have outbound and at least one matcher key
    const auto rules = routeObj.value("rules").toArray();
    for (const auto &rv : rules)
    {
        const auto r = rv.toObject();
        REQUIRE(r.contains("outbound"));
        if (r.contains("geosite")) REQUIRE(r.value("geosite").isArray());
        if (r.contains("geoip")) REQUIRE(r.value("geoip").isArray());
        // allow domain/ip_cidr/etc. exist as arrays when present
        if (r.contains("domain")) REQUIRE(r.value("domain").isArray());
        if (r.contains("ip_cidr")) REQUIRE(r.value("ip_cidr").isArray());
    }

    // DNS details (1.12.0+ format expected)
    REQUIRE(jsonObj.contains("dns"));
    const auto dnsObj = jsonObj.value("dns").toObject();
    // servers or rules should exist depending on migrator default
    REQUIRE(dnsObj.keys().size() > 0);
    if (dnsObj.contains("servers"))
    {
        REQUIRE(dnsObj.value("servers").isArray());
        const auto servers = dnsObj.value("servers").toArray();
        // allow empty but if present, entries should be string or object
        if (!servers.isEmpty())
        {
            auto s0 = servers.first();
            REQUIRE(s0.isString() || s0.isObject());
        }
    }
    if (dnsObj.contains("rules"))
    {
        REQUIRE(dnsObj.value("rules").isArray());
    }
}
