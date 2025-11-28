#include "XeCommandStream.hpp"
#include "XeService.hpp"
#include "XeBootArgs.hpp"

void XeCommandStream::logRcs0State() const {
  XeLog("XeCS: logRcs0State stub (strictsafe mode)\n");
}

IOReturn XeCommandStream::submitNoop(IOBufferMemoryDescriptor* /*bo*/) {
  XeLog("XeCS: submitNoop stub (strictsafe mode)\n");
  return kIOReturnNotReady;
}
