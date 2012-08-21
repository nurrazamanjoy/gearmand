/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * 
 *  Gearmand client and server library.
 *
 *  Copyright (C) 2012 Data Differential, http://datadifferential.com/
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *
 *      * Redistributions of source code must retain the above copyright
 *  notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *  copyright notice, this list of conditions and the following disclaimer
 *  in the documentation and/or other materials provided with the
 *  distribution.
 *
 *      * The names of its contributors may not be used to endorse or
 *  promote products derived from this software without specific prior
 *  written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#pragma once

struct Client
{
  struct Options {
    bool allocated;
    bool non_blocking;
    bool unbuffered_result;
    bool no_new;
    bool free_tasks;

    Options():
      allocated(true),
      non_blocking(false),
      unbuffered_result(false),
      no_new(false),
      free_tasks(false)
    {
    }
  } options;
  enum gearman_client_t state;
  uint32_t new_tasks;
  uint32_t running_tasks;
  uint32_t task_count;
  void *context;
  gearman_connection_st *con;
  gearman_task_st *task;
  gearman_task_st *task_list;
  gearman_task_context_free_fn *task_context_free_fn;
  struct gearman_universal_st universal;
  struct gearman_actions_t actions;
  gearman_job_handle_t _do_handle; // Backwards compatible
  gearman_client_st* _shell;


  Client(gearman_client_st* shell_) :
    state(GEARMAN_CLIENT_STATE_IDLE),
    new_tasks(0),
    running_tasks(0),
    task_count(0),
    context(NULL),
    con(NULL),
    task(NULL),
    task_list(NULL),
    task_context_free_fn(NULL),
    _shell(shell_)
  {
    gearman_client_clear_fn(_shell);
  }

  gearman_client_st* shell()
  {
    return _shell;
  }
};
