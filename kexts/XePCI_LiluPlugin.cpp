// kexts/XePCI_LiluPlugin.cpp — Convert skeleton into a Lilu-style plugin
// Requires Lilu headers in include path. Bundle ID and plist must mark this as a Lilu plugin.
// This keeps your existing PCI/MMIO logic, but moves init into Lilu's start hooks.

#include <Headers/plugin_start.hpp>    // Lilu
#include <Headers/kern_api.hpp>        // Lilu
#include <IOKit/pci/IOPCIDevice.h>
#include <IOKit/IOLib.h>

static const char *kPluginId   = "org.yourorg.XePCI";
static const char *kFindClass  = "IOPCIDevice";
static const uint16_t kVendor  = 0x8086;
// Use your exact product id, e.g. 0xA7A1:
static const uint16_t kProduct = 0xA7A1;

class XeService : public IOService {
  OSDeclareDefaultStructors(XeService)
private:
  IOPCIDevice *pci {nullptr};
  IOMemoryMap *bar0 {nullptr};
  volatile uint32_t *mmio {nullptr};
public:
  bool init(OSDictionary *prop) override {
    if (!super::init(prop)) return false;
    return true;
  }
  IOService *probe(IOService *provider, SInt32 *score) override {
    auto dev = OSDynamicCast(IOPCIDevice, provider);
    if (!dev) return nullptr;
    uint16_t v = dev->configRead16(kIOPCIConfigVendorID);
    uint16_t d = dev->configRead16(kIOPCIConfigDeviceID);
    if (v == kVendor && d == kProduct) *score += 1000;
    return super::probe(provider, score);
  }
  bool start(IOService *provider) override {
    if (!super::start(provider)) return false;
    pci = OSDynamicCast(IOPCIDevice, provider);
    if (!pci) return false;
    pci->setMemoryEnable(true);
    pci->setIOEnable(true);
    pci->setBusMasterEnable(true);
    bar0 = pci->mapDeviceMemoryWithRegister(kIOPCIConfigBaseAddress0);
    if (!bar0) return false;
    mmio = reinterpret_cast<volatile uint32_t *>(bar0->getVirtualAddress());
    if (!mmio) return false;
    IOLog("XeLilu: BAR0 @ %p\n", (void*)mmio);
    // Safe read-only probe
    uint32_t r0 = mmio[0x0/4];
    IOLog("XeLilu: reg[0x0]=0x%08x\n", r0);
    registerService();
    return true;
  }
  void stop(IOService *provider) override {
    if (bar0) { bar0->release(); bar0=nullptr; mmio=nullptr; }
    super::stop(provider);
  }
};

OSDefineMetaClassAndStructors(XeService, IOService)

// Lilu plugin interface
static void pluginStart() {
  DBGLOG("XeLilu", "pluginStart");
}

static void pluginStop() {
  DBGLOG("XeLilu", "pluginStop");
}

PluginConfiguration ADDPR(config) = {
  kPluginId,
  parseModuleVersion("1.0.0"),
  LiluAPI::AllowNormal | LiluAPI::AllowInstallerRecovery,
  nullptr, 0,
  nullptr, 0,
  pluginStart,
  pluginStop
};
