// kexts/XePCI_LiluPlugin.cpp — Convert skeleton into a Lilu-style plugin
// Requires Lilu headers in include path. Bundle ID and plist must mark this as a Lilu plugin.
// This keeps your existing PCI/MMIO logic, but moves init into Lilu's start hooks.

#include <Headers/plugin_start.hpp>    // Lilu
#include <Headers/kern_api.hpp>        // Lilu
#include "XeService.hpp"

static const char *bootargOff[] {
	"-xepcioff"
};

static const char *bootargDebug[] {
	"-xepcidbg"
};

static const char *bootargBeta[] {
	"-xepcibeta"
};

// Lilu plugin interface
static void pluginStart() {
	DBGLOG("XePCI", "pluginStart");
}

PluginConfiguration ADDPR(config) {
	xStringify(PRODUCT_NAME),
	parseModuleVersion(xStringify(MODULE_VERSION)),
	LiluAPI::AllowNormal | LiluAPI::AllowInstallerRecovery | LiluAPI::AllowSafeMode,
	bootargOff,
	arrsize(bootargOff),
	bootargDebug,
	arrsize(bootargDebug),
	bootargBeta,
	arrsize(bootargBeta),
	KernelVersion::SnowLeopard,
	KernelVersion::Ventura,
	[]() {
		pluginStart();
	}
};
