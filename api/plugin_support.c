/*
  This file is part of MAMBO, a low-overhead dynamic binary modification tool:
      https://github.com/beehive-lab/mambo

  Copyright 2013-2016 Cosmin Gorgovan <cosmin at linux-geek dot org>
  Copyright 2017 The University of Manchester

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#include <stdio.h>
#include <assert.h>
#include <sys/mman.h>
#include <stdarg.h>

#include "../dbm.h"
#include "../common.h"
#include "helpers.h"

#ifdef PLUGINS_NEW

/* Plugin management */
mambo_context *mambo_register_plugin(void) {
  int index = global_data.free_plugin++;
  static mambo_context tmp_ctx;

  if (index >= MAX_PLUGIN_NO) {
    return NULL;
  }

  set_mambo_context(&tmp_ctx, NULL, PLUGIN_REG);
  tmp_ctx.plugin_id = index;

  return &tmp_ctx;
}

/* Callback management */
int __mambo_register_cb(mambo_context *ctx, mambo_cb_idx cb_idx, mambo_callback cb) {
  unsigned int p_id = ctx->plugin_id;

  if (cb_idx >= CALLBACK_MAX_IDX || cb_idx < 0) {
    return MAMBO_INVALID_CB;
  }

  if (p_id >= MAX_PLUGIN_NO) {
    return MAMBO_INVALID_PLUGIN_ID;
  }

  if (global_data.plugins[p_id].cbs[cb_idx] != NULL) {
    return MAMBO_CB_ALREADY_SET;
  }

  global_data.plugins[p_id].cbs[cb_idx] = cb;

  return MAMBO_SUCCESS;
}

int mambo_register_pre_inst_cb(mambo_context *ctx, mambo_callback cb) {
  return __mambo_register_cb(ctx, PRE_INST_C, cb);
}

int mambo_register_post_inst_cb(mambo_context *ctx, mambo_callback cb) {
  return __mambo_register_cb(ctx, POST_INST_C, cb);
}

int mambo_register_pre_basic_block_cb(mambo_context *ctx, mambo_callback cb) {
  return __mambo_register_cb(ctx, PRE_BB_C, cb);
}

int mambo_register_post_basic_block_cb(mambo_context *ctx, mambo_callback cb) {
  return __mambo_register_cb(ctx, POST_BB_C, cb);
}

int mambo_register_pre_fragment_cb(mambo_context *ctx, mambo_callback cb) {
  return __mambo_register_cb(ctx, PRE_FRAGMENT_C, cb);
}

int mambo_register_post_fragment_cb(mambo_context *ctx, mambo_callback cb) {
  return __mambo_register_cb(ctx, POST_FRAGMENT_C, cb);
}

int mambo_register_pre_syscall_cb(mambo_context *ctx, mambo_callback cb) {
  return __mambo_register_cb(ctx, PRE_SYSCALL_C, cb);
}

int mambo_register_post_syscall_cb(mambo_context *ctx, mambo_callback cb) {
  return __mambo_register_cb(ctx, POST_SYSCALL_C, cb);
}

int mambo_register_pre_thread_cb(mambo_context *ctx, mambo_callback cb) {
  return __mambo_register_cb(ctx, PRE_THREAD_C, cb);
}

int mambo_register_post_thread_cb(mambo_context *ctx, mambo_callback cb) {
  return __mambo_register_cb(ctx, POST_THREAD_C, cb);
}

int mambo_register_exit_cb(mambo_context *ctx, mambo_callback cb) {
  return __mambo_register_cb(ctx, EXIT_C, cb);
}

/* Access plugin data */
int mambo_set_plugin_data(mambo_context *ctx, void *data) {
  unsigned int p_id = ctx->plugin_id;
  if (p_id >= global_data.free_plugin) {
    return MAMBO_INVALID_PLUGIN_ID;
  }
  global_data.plugins[p_id].data = data;
  return MAMBO_SUCCESS;
}

void *mambo_get_plugin_data(mambo_context *ctx) {
  unsigned int p_id = ctx->plugin_id;
  if (p_id >= global_data.free_plugin) {
    return NULL;
  }
  return global_data.plugins[p_id].data;
}

int mambo_set_thread_plugin_data(mambo_context *ctx, void *data) {
  unsigned int p_id = ctx->plugin_id;
  if (p_id >= global_data.free_plugin) {
    return MAMBO_INVALID_PLUGIN_ID;
  }
  if (ctx->thread_data == NULL) {
    return MAMBO_INVALID_THREAD;
  }
  ctx->thread_data->plugin_priv[p_id] = data;
  return MAMBO_SUCCESS;
}

void *mambo_get_thread_plugin_data(mambo_context *ctx) {
  unsigned int p_id = ctx->plugin_id;
  if (p_id >= global_data.free_plugin) {
    return NULL;
  }
  if (ctx->thread_data == NULL) {
    return NULL;
  }
  return ctx->thread_data->plugin_priv[p_id];
}

/* Memory management */
void *mambo_alloc(mambo_context *ctx, size_t size) {
  return mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
}

void mambo_free(mambo_context *ctx, void *ptr) {
}

/* Other */
int mambo_get_inst(mambo_context *ctx) {
  return ctx->code.inst;
}

inst_set mambo_get_inst_type(mambo_context *ctx) {
  return ctx->code.inst_type;
}

int mambo_get_fragment_id(mambo_context *ctx) {
  return ctx->code.fragment_id;
}

cc_type mambo_get_fragment_type(mambo_context *ctx) {
  return ctx->code.fragment_type;
}

int mambo_get_inst_len(mambo_context *ctx) {
  int inst = mambo_get_inst(ctx);
  // Not an instruction event
  if (inst == -1) {
    return -1;
  }
#ifdef __arm__
  if (mambo_get_inst_type(ctx) == ARM_INST) {
    return 4;
  } else {
    return (inst < THUMB_ADC32) ? 2 : 4;
  }
#elif __aarch64__
  return 4;
#endif
}

void *mambo_get_source_addr(mambo_context *ctx) {
  return ctx->code.read_address;
}

void *mambo_get_cc_addr(mambo_context *ctx) {
  return ctx->code.write_p;
}

void mambo_set_cc_addr(mambo_context *ctx, void *addr) {
  assert(ctx->code.write_p != NULL);
  ctx->code.write_p = addr;
}

int mambo_get_thread_id(mambo_context *ctx) {
  return ctx->thread_data->tid;
}

mambo_cond mambo_get_cond(mambo_context *ctx) {
  return ctx->code.cond;
}

bool mambo_is_cond(mambo_context *ctx) {
  return ctx->code.cond != AL;
}

mambo_cond mambo_get_inverted_cond(mambo_context *ctx, mambo_cond cond) {
  return invert_cond(cond & 0xF);
}

void mambo_replace_inst(mambo_context *ctx) {
  ctx->code.replace = true;
}

/* Allows scratch registers to be shared by multiple plugins
  This will likely be modified in the future to allocate dead
  application registers if available.
*/
int mambo_get_scratch_regs(mambo_context *ctx, int count, ...) {
  int *regp;
  int min_pushed_reg = 8; // subject to change; selected for thumb-16 push/pops
  int allocated_regs = 0;
  uint32_t to_push = 0;

  va_list args;
  va_start(args, count);

  if (ctx->code.pushed_regs) {
    min_pushed_reg = next_reg_in_list(ctx->code.pushed_regs, 0);
  }

  for (int i = 0; i < count; i++) {
    regp = va_arg(args, int *);
    int reg = next_reg_in_list(ctx->code.available_regs, 0);
    if (reg != reg_invalid) {
      ctx->code.available_regs &= ~(1 << reg);
    } else {
      min_pushed_reg--;
      if (min_pushed_reg >= 0) {
        to_push |= 1 << min_pushed_reg;
        reg = min_pushed_reg;
      }
    }
    if (reg >= 0 && reg < reg_invalid) {
      *regp = reg;
      allocated_regs++;
    } else {
      break;
    }
  }

  ctx->code.pushed_regs |= to_push;
  if (to_push) {
    emit_push(ctx, to_push);
  }

  return allocated_regs;
}

int mambo_get_scratch_reg(mambo_context *ctx, int *regp) {
  return mambo_get_scratch_regs(ctx, 1, regp);
}

int mambo_free_scratch_regs(mambo_context *ctx, uint32_t regs) {
  if ((regs & ctx->code.pushed_regs) != regs) {
    return -1;
  }
  ctx->code.available_regs |= regs;
  return 0;
}

int mambo_free_scratch_reg(mambo_context *ctx, int reg) {
  return mambo_free_scratch_reg(ctx, 1 << reg);
}

/* Syscall helpers */
int mambo_syscall_get_no(mambo_context *ctx, uintptr_t *no) {
  if (ctx->event_type == PRE_SYSCALL_C ||
      ctx->event_type == POST_SYSCALL_C) {
    *no = ctx->syscall.number;
    return 0;
  }
  return -1;
}

void mambo_syscall_get_args(mambo_context *ctx, uintptr_t **args) {
  *args = NULL;
  if (ctx->event_type == PRE_SYSCALL_C) {
    *args = ctx->syscall.regs;
  }
}

int mambo_syscall_bypass(mambo_context *ctx) {
  if (ctx->event_type == PRE_SYSCALL_C) {
    assert(ctx->syscall.replace == false);
    ctx->syscall.replace = true;
    // Initialise the return value
    // This is similar to leaving all registers untouched, as opposed to a NOP syscall
    ctx->syscall.ret = ctx->syscall.regs[reg0];
    return 0;
  }
  return -1;
}

int mambo_syscall_get_return(mambo_context *ctx, uintptr_t *ret) {
  if (ctx->event_type == POST_SYSCALL_C) {
    *ret = ctx->syscall.regs[reg0];
    return 0;
  }
  if (ctx->event_type == PRE_SYSCALL_C && ctx->syscall.replace) {
    *ret = ctx->syscall.ret;
    return 0;
  }
  return -1;
}

int mambo_syscall_set_return(mambo_context *ctx, uintptr_t ret) {
  if (ctx->event_type == POST_SYSCALL_C) {
    ctx->syscall.regs[reg0] = ret;
    return 0;
  }
  // This way we preserve the arguments even after setting a return
  if (ctx->event_type == PRE_SYSCALL_C && ctx->syscall.replace) {
    ctx->syscall.ret = ret;
    return 0;
  }
  return -1;
}
#endif