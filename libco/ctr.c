/* 3DS (ARM11, hard-float): like arm.c, but also keeps the callee-saved VFP registers d8-d15,
   which the hard-float calling convention expects to survive a call. needs no executable data:
   the switch is ordinary compiled code. */
#define LIBCO_C
#include "libco.h"
#include "settings.h"

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* r4-r11, sp, lr (10 words), then d8-d15 (16 words) */
static thread_local unsigned long co_active_buffer[64];
static thread_local cothread_t co_active_handle = 0;

__attribute__((naked, target("arm"))) static void co_swap(cothread_t to, cothread_t from) {
  __asm__ volatile(
    "stmia r1!, {r4-r11, sp, lr}\n\t"
    "vstmia r1!, {d8-d15}\n\t"
    "ldmia r0!, {r4-r11, sp, lr}\n\t"
    "vldmia r0!, {d8-d15}\n\t"
    "bx lr\n\t"
  );
}

cothread_t co_active() {
  if(!co_active_handle) co_active_handle = &co_active_buffer;
  return co_active_handle;
}

cothread_t co_derive(void* memory, unsigned int size, void (*entrypoint)(void)) {
  unsigned long* handle;
  if(!co_active_handle) co_active_handle = &co_active_buffer;

  if((handle = (unsigned long*)memory)) {
    unsigned int offset = (size & ~15);
    unsigned long* p = (unsigned long*)((unsigned char*)handle + offset);
    for(unsigned int n = 0; n < 26; n++) handle[n] = 0;
    handle[8] = (unsigned long)p;
    handle[9] = (unsigned long)entrypoint;
  }

  return handle;
}

cothread_t co_create(unsigned int size, void (*entrypoint)(void)) {
  void* memory = malloc(size);
  if(!memory) return (cothread_t)0;
  return co_derive(memory, size, entrypoint);
}

void co_delete(cothread_t handle) {
  free(handle);
}

void co_switch(cothread_t handle) {
  cothread_t co_previous_handle = co_active_handle;
  co_swap(co_active_handle = handle, co_previous_handle);
}

int co_serializable() {
  return 1;
}

#ifdef __cplusplus
}
#endif
