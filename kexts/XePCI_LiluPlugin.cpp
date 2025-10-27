// kexts/XePCI_LiluPlugin.cpp — Convert skeleton into a Lilu-style plugin
// Requires Lilu headers in include path. Bundle ID and plist must mark this as a Lilu plugin.
// This keeps your existing PCI/MMIO logic, but moves init into Lilu's start hooks.

#include <Headers/plugin_start.hpp>    // Lilu
#include <Headers/kern_api.hpp>        // Lilu
#include "XeService.hpp"

static const char *kPluginId   = "org.yourorg.XePCI";

// Lilu plugin interface
static void pluginStart() {
  DBGLOG("XeLilu", "pluginStart");
}

static void pluginStop() {
  DBGLOG("XeLilu", "pluginStop");
}

PluginConfiguration ADDPR(config) = [] {
  PluginConfiguration config{};

  config.product = kPluginId;
  config.version = parseModuleVersion("1.0.0");
  config.compatibility = LiluAPI::AllowNormal | LiluAPI::AllowInstallerRecovery;
  config.start = pluginStart;
  config.stop = pluginStop;

  return config;
}();
