// userspace/xectl.c — updated to match class "XeService" and method indices

// Build: clang xectl.c -framework IOKit -framework CoreFoundation -o xectl
// Usage: sudo ./xectl info | regdump | noop | mkbuf [bytes]

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static const char *kServiceClass = "XeService";

enum {
  kMethodCreateBuffer = 0,
  kMethodSubmit       = 1,
  kMethodWait         = 2,
  kMethodReadReg      = 3,
  kMethodGetDeviceInfo = 4,
  kMethodGetGTConfig  = 5,
};

static io_connect_t open_connection(void) {
  CFMutableDictionaryRef match = IOServiceMatching(kServiceClass);
  if (!match) { fprintf(stderr, "No matching dict\n"); exit(1); }
  io_iterator_t it = 0;
  kern_return_t kr = IOServiceGetMatchingServices(kIOMasterPortDefault, match, &it);
  if (kr != KERN_SUCCESS) { fprintf(stderr, "No service iterator\n"); exit(1); }
  io_service_t svc = IOIteratorNext(it);
  IOObjectRelease(it);
  if (!svc) { fprintf(stderr, "Service not found (XeService)\n"); exit(1); }
  io_connect_t conn = IO_OBJECT_NULL;
  kr = IOServiceOpen(svc, mach_task_self(), 0, &conn);
  IOObjectRelease(svc);
  if (kr != KERN_SUCCESS) { fprintf(stderr, "Open failed: 0x%x\n", kr); exit(1); }
  return conn;
}

static void cmd_info(io_connect_t c) {
  printf("Connected to %s; handle=%u\n", kServiceClass, c);
  
  // Get device info
  uint64_t info[4]; uint32_t infoCnt = 4;
  kern_return_t kr = IOConnectCallMethod(c, kMethodGetDeviceInfo, NULL, 0, NULL, 0, info, &infoCnt, NULL, 0);
  if (kr == KERN_SUCCESS && infoCnt >= 4) {
    printf("Vendor ID: 0x%04x\n", (uint32_t)info[0]);
    printf("Device ID: 0x%04x\n", (uint32_t)info[1]);
    printf("Revision:  0x%02x\n", (uint32_t)info[2]);
    printf("Accel Ready: %s\n", info[3] ? "Yes" : "No (prepared)");
  } else {
    printf("Failed to get device info\n");
  }
}

static void cmd_regdump(io_connect_t c) {
  uint64_t out[8]; uint32_t outCnt = 8;
  kern_return_t kr = IOConnectCallMethod(c, kMethodReadReg, NULL, 0, NULL, 0, out, &outCnt, NULL, 0);
  if (kr != KERN_SUCCESS) { fprintf(stderr, "regdump failed: 0x%x\n", kr); return; }
  for (uint32_t i = 0; i < outCnt; ++i) printf("reg[%u]=0x%08x\n", i, (uint32_t)out[i]);
}

static void cmd_noop(io_connect_t c) {
  kern_return_t kr = IOConnectCallMethod(c, kMethodSubmit, NULL, 0, NULL, 0, NULL, NULL, NULL, 0);
  if (kr != KERN_SUCCESS) { fprintf(stderr, "submit failed: 0x%x\n", kr); return; }
  uint64_t in[1] = { 1000 }; size_t inCnt = 1;
  kr = IOConnectCallMethod(c, kMethodWait, in, inCnt, NULL, 0, NULL, NULL, NULL, 0);
  if (kr != KERN_SUCCESS) { fprintf(stderr, "wait failed: 0x%x\n", kr); return; }
  printf("NOOP completed (stub)\n");
}

static void cmd_mkbuf(io_connect_t c, uint32_t bytes) {
  uint64_t in[1] = { bytes }; size_t inCnt = 1;
  uint64_t out[1]; uint32_t outCnt = 1;
  kern_return_t kr = IOConnectCallMethod(c, kMethodCreateBuffer, in, inCnt, NULL, 0, out, &outCnt, NULL, 0);
  if (kr != KERN_SUCCESS) { fprintf(stderr, "createBuffer failed: 0x%x\n", kr); return; }
  printf("Created buffer cookie=0x%llx (size=%u)\n", (unsigned long long)out[0], bytes);
}

static void cmd_gtconfig(io_connect_t c) {
  uint64_t config[4]; uint32_t configCnt = 4;
  kern_return_t kr = IOConnectCallMethod(c, kMethodGetGTConfig, NULL, 0, NULL, 0, config, &configCnt, NULL, 0);
  if (kr != KERN_SUCCESS) { fprintf(stderr, "gtconfig failed: 0x%x\n", kr); return; }
  if (configCnt >= 4) {
    printf("GT_THREAD_STATUS:        0x%08x\n", (uint32_t)config[0]);
    printf("GT_GEOMETRY_DSS_ENABLE:  0x%08x\n", (uint32_t)config[1]);
    printf("FORCEWAKE_ACK_GT:        0x%08x\n", (uint32_t)config[2]);
    printf("Ring Status:             0x%08x\n", (uint32_t)config[3]);
  }
}

int main(int argc, char **argv) {
  if (argc < 2) { 
    fprintf(stderr, "usage: %s [info|regdump|noop|mkbuf BYTES|gtconfig]\n", argv[0]); 
    return 1; 
  }
  io_connect_t c = open_connection();
  if (!strcmp(argv[1], "info"))         cmd_info(c);
  else if (!strcmp(argv[1], "regdump")) cmd_regdump(c);
  else if (!strcmp(argv[1], "noop"))    cmd_noop(c);
  else if (!strcmp(argv[1], "mkbuf") && argc >= 3) cmd_mkbuf(c, (uint32_t)strtoul(argv[2], NULL, 0));
  else if (!strcmp(argv[1], "gtconfig")) cmd_gtconfig(c);
  else fprintf(stderr, "unknown cmd\n");
  IOServiceClose(c);
  return 0;
}
