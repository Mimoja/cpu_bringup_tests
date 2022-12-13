#include <efi.h>
#include <efilib.h>
#include <stdbool.h>

// make the compiler happy 
int _fltused = 0;

EFI_SYSTEM_TABLE *SystemTable;
extern int* ap_trampoline;

void sleep()
{
  volatile int y = 0b1011000101;
  for (int i = 0; i < INT32_MAX / 3; i++)
  {
    y = ~y;
  }
}


// https://wiki.osdev.org/APIC
#define APIC_INT_CMD_LOW 0x300
#define APIC_INT_CMD_HIGH 0x310
#define CPUID_FEAT_EDX_APIC (1 << 9)
#define IA32_APIC_BASE_MSR 0x1B
#define IA32_APIC_BASE_MSR_BSP 0x100 // Processor is a BSP
#define IA32_APIC_BASE_MSR_ENABLE 0x800

// https://wiki.osdev.org/CPUID
static inline void cpuid(int code, uint32_t *a, uint32_t *d)
{
  asm volatile("cpuid"
               : "=a"(*a), "=d"(*d)
               : "a"(code)
               : "ecx", "ebx");
}

bool supports_apic()
{
  uint32_t eax, edx;
  cpuid(1, &eax, &edx);
  return edx & CPUID_FEAT_EDX_APIC;
}

void cpuGetMSR(uint32_t msr, uint32_t *lo, uint32_t *hi)
{
  asm volatile("rdmsr"
               : "=a"(*lo), "=d"(*hi)
               : "c"(msr));
}

uintptr_t cpu_get_apic_base()
{
  uint32_t eax, edx;
  cpuGetMSR(IA32_APIC_BASE_MSR, &eax, &edx);

  return (eax & 0xfffff000);
}

void writeAPIC(uint32_t reg, uint32_t value, uint32_t mask)
{
  uint32_t volatile *ptr;
  ptr = (uint32_t volatile *) cpu_get_apic_base() + reg;
  // https://wiki.osdev.org/Symmetric_Multiprocessing
  *ptr = (*ptr & mask) | value;
}

void sendInitIPI(int target)
{
  writeAPIC(APIC_INT_CMD_HIGH, target << 24, 0x00ffffff);
  writeAPIC(APIC_INT_CMD_LOW, 0xC500, 0xfff00000);
}

void sendDeassertIPI(int target)
{
  writeAPIC(APIC_INT_CMD_HIGH, target << 24, 0x00ffffff);
  writeAPIC(APIC_INT_CMD_LOW, 0x8500, 0xfff00000);
}

void sendStartupIPI(int target, intptr_t entry)
{
  writeAPIC(APIC_INT_CMD_HIGH, target << 24, 0x00ffffff);
  writeAPIC(APIC_INT_CMD_LOW, 0x600 | entry, 0xfff0f800);
}

void waitForDelivery()
{
  volatile void *ptr = (uint32_t volatile *)cpu_get_apic_base();
  int i = 0;
  while (*((volatile uint32_t *)(ptr + 0x300)) & (1 << 12))
  {
    __asm__ __volatile__("pause"
                         :
                         :
                         : "memory");
    i++;
  };
  if (i)
    Print(L"Waited %d cycles for delivery\n", i);
}

volatile uint8_t aprunning = 0; 

// this C code can be anywhere you want it, no relocation needed
void secondary_ap_main(int apicid) {

  Print(L"AP 0x%lx has landed (%d/%d) \n", apicid, aprunning, 16);

	while(1){
    sleep();
  }
}

EFI_STATUS
EFIAPI
efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *sys)
{
  SystemTable = sys;
  InitializeLib(ImageHandle, SystemTable);

  EFI_LOADED_IMAGE *LoadedImage = NULL;
  EFI_STATUS status;

  status = uefi_call_wrapper(SystemTable->BootServices->HandleProtocol,
                             3,
                             ImageHandle,
                             &LoadedImageProtocol,
                             (void **)&LoadedImage);
  if (EFI_ERROR(status))
  {
    Print(L"handleprotocol: %r\n", status);
  }

  Print(L"Image base: 0x%lx\n", LoadedImage->ImageBase);
  Print(L"APIC 0 base: 0x%lx\n", cpu_get_apic_base());

  ASSERT(supports_apic());

  memcpy((void*)0x8000, &ap_trampoline, 4096);

  for (int apic = 1; apic < 16; apic++)
  {
    Print(L"IPI CPU %d\n", apic);

    sendInitIPI(apic);
    waitForDelivery();
    sendDeassertIPI(apic);
    waitForDelivery();
  }

  sleep();
  writeAPIC(0x280, 0, 0); // clear errors

  for (int apic = 1; apic < 16; apic++)
  {
    Print(L"IPI CPU %d to 0x%lx\n", apic, 0x8000);
    sendStartupIPI(apic, 0x8);
    waitForDelivery();
  }
  Print(L"Done!\n");

  while (1)
  {
    sleep();
  }
  return EFI_SUCCESS;
}