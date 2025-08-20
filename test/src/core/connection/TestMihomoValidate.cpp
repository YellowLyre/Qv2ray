#include "catch.hpp"
#include "Common.hpp"

#include "core/kernel/PolyCoreKernelInterface.hpp"

#include <QTemporaryFile>
#include <QTextStream>

using namespace PolyCore::core::kernel;

static QString writeTemp(const QString &content)
{
    QTemporaryFile tmp;
    tmp.setAutoRemove(false);
    REQUIRE(tmp.open());
    QTextStream ts(&tmp);
    ts << content;
    tmp.flush();
    const QString path = tmp.fileName();
    tmp.close();
    return path;
}

TEST_CASE("Mihomo YAML validation basic structure", "[mihomo][validate]")
{
    MihomoKernelAdapter adapter;

    // Valid: has proxy-groups and rules
    const QString validYaml = R"(proxy-groups:
  - name: default
    type: select
    proxies: [DIRECT]
rules:
  - MATCH,default
)";
    const auto validPath = writeTemp(validYaml);
    REQUIRE(adapter.ValidateConfig(validPath) == true);

    // Valid: has proxies and rules
    const QString validYaml2 = R"(proxies:
  - name: a
    type: socks5
    server: 127.0.0.1
    port: 1080
rules:
  - MATCH,DIRECT
)";
    const auto validPath2 = writeTemp(validYaml2);
    REQUIRE(adapter.ValidateConfig(validPath2) == true);

    // Invalid: missing rules
    const QString invalidYaml = R"(proxies:
  - name: a
    type: socks5
    server: 127.0.0.1
    port: 1080
)";
    const auto invalidPath = writeTemp(invalidYaml);
    REQUIRE(adapter.ValidateConfig(invalidPath) == false);

    // Invalid: empty
    const QString emptyPath = writeTemp("");
    REQUIRE(adapter.ValidateConfig(emptyPath) == false);
}
