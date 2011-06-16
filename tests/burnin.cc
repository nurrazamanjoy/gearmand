/* Gearman server and library
 * Copyright (C) 2008 Brian Aker, Eric Day
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

#include "config.h"

#if defined(NDEBUG)
# undef NDEBUG
#endif

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <libgearman/gearman.h>

#include <libtest/test.h>
#include <libtest/server.h>
#include <libtest/worker.h>

#define CLIENT_TEST_PORT 32143

#define DEFAULT_WORKER_NAME "burnin"

struct client_test_st {
  gearman_client_st client;
  pid_t gearmand_pid;
  struct worker_handle_st *handle;

  client_test_st():
    gearmand_pid(-1),
    handle(NULL)
  { }
};

struct client_context_st {
  int latch;
  size_t min_size;
  size_t max_size;
  size_t num_tasks;
  size_t count;
  char *blob;

  client_context_st():
    latch(0),
    min_size(1024),
    max_size(1024 *2),
    num_tasks(20),
    count(2000),
    blob(NULL)
  { }
};

void *world_create(test_return_t *error);
test_return_t world_destroy(void *object);

#ifndef __INTEL_COMPILER
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

static test_return_t burnin_test(void *object)
{
  gearman_client_st *client= (gearman_client_st *)object;

  struct client_context_st *context= (struct client_context_st *)gearman_client_context(client);

  // This sketchy, don't do this in your own code.
  gearman_task_st *tasks= (gearman_task_st *)calloc(context->num_tasks, sizeof(gearman_task_st));
  test_true_got(tasks, strerror(errno));

  test_true_got(gearman_success(gearman_client_echo(client, gearman_literal_param("echo_test"))), gearman_client_error(client));

  do
  {
    for (uint32_t x= 0; x < context->num_tasks; x++)
    {
      size_t blob_size= 0;

      if (context->min_size == context->max_size)
      {
        blob_size= context->max_size;
      }
      else
      {
        blob_size= (size_t)rand();

        if (context->max_size > RAND_MAX)
          blob_size*= (size_t)(rand() + 1);

        blob_size= (blob_size % (context->max_size - context->min_size)) + context->min_size;
      }

      gearman_task_st *task_ptr;
      gearman_return_t ret;
      if (context->latch)
      {
        task_ptr= gearman_client_add_task_background(client, &(tasks[x]),
                                                     NULL, DEFAULT_WORKER_NAME, NULL,
                                                     (void *)context->blob, blob_size, &ret);
      }
      else
      {
        task_ptr= gearman_client_add_task(client, &(tasks[x]), NULL,
                                          DEFAULT_WORKER_NAME, NULL, (void *)context->blob, blob_size,
                                          &ret);
      }

      test_true_got(gearman_success(ret), gearman_client_error(client));
      test_truth(task_ptr);
    }

    gearman_return_t ret= gearman_client_run_tasks(client);
    for (uint32_t x= 0; x < context->num_tasks; x++)
    {
      test_compare(GEARMAN_TASK_STATE_FINISHED, tasks[x].state);
      test_compare(GEARMAN_SUCCESS, tasks[x].result_rc);
    }
    test_compare(0, client->new_tasks);

    test_true_got(gearman_success(ret), gearman_client_error(client));

    for (uint32_t x= 0; x < context->num_tasks; x++)
    {
      gearman_task_free(&(tasks[x]));
    }
  } while (context->count--);

  free(tasks);

  context->latch++;

  return TEST_SUCCESS;
}

static test_return_t setup(void *object)
{
  gearman_client_st *client= (gearman_client_st *)object;

  struct client_context_st *context= new client_context_st;
  test_true_got(context, strerror(errno));

  context->blob= (char *)malloc(context->max_size);
  test_true_got(context->blob, strerror(errno));
  memset(context->blob, 'x', context->max_size); 

  gearman_client_set_context(client, context);

  return TEST_SUCCESS;
}

static test_return_t cleanup(void *object)
{
  gearman_client_st *client= (gearman_client_st *)object;

  struct client_context_st *context= (struct client_context_st *)gearman_client_context(client);

  free(context->blob);
  delete(context);

  return TEST_SUCCESS;
}


static void *worker_fn(gearman_job_st *, void *,
                       size_t *result_size, gearman_return_t *ret_ptr)
{
  result_size= 0;
  *ret_ptr= GEARMAN_SUCCESS;
  return NULL;
}

void *world_create(test_return_t *error)
{
  pid_t gearmand_pid;

  /**
   *  @TODO We cast this to char ** below, which is evil. We need to do the
   *  right thing
   */
  const char *argv[1]= { "client_gearmand" };

  client_test_st *test= new client_test_st;
  if (not test)
  {
    *error= TEST_MEMORY_ALLOCATION_FAILURE;
    return NULL;
  }

  /**
    We start up everything before we allocate so that we don't have to track memory in the forked process.
  */
  test->gearmand_pid= gearmand_pid= test_gearmand_start(CLIENT_TEST_PORT, 1, argv);
  if (test->gearmand_pid == -1)
  {
    *error= TEST_FAILURE;
    return NULL;
  }

  test->handle= test_worker_start(CLIENT_TEST_PORT, DEFAULT_WORKER_NAME, worker_fn, NULL, gearman_worker_options_t());
  if (not test->handle)
  {
    *error= TEST_FAILURE;
    return NULL;
  }

  if (not gearman_client_create(&(test->client)))
  {
    *error= TEST_FAILURE;
    return NULL;
  }

  if (gearman_failed(gearman_client_add_server(&(test->client), NULL, CLIENT_TEST_PORT)))
  {
    *error= TEST_FAILURE;
    return NULL;
  }

  *error= TEST_SUCCESS;

  return (void *)test;
}

test_return_t world_destroy(void *object)
{
  client_test_st *test= (client_test_st *)object;
  gearman_client_free(&(test->client));
  test_gearmand_stop(test->gearmand_pid);
  test_worker_stop(test->handle);
  delete test;

  return TEST_SUCCESS;
}


test_st tests[] ={
  {"burnin", 0, burnin_test },
//  {"burnin_background", 0, burnin_test },
  {0, 0, 0}
};


collection_st collection[] ={
  {"burnin", setup, cleanup, tests},
  {0, 0, 0, 0}
};

typedef test_return_t (*libgearman_test_callback_fn)(gearman_client_st *);
static test_return_t _runner_default(libgearman_test_callback_fn func, client_test_st *container)
{
  if (func)
  {
    return func(&container->client);
  }
  else
  {
    return TEST_SUCCESS;
  }
}

static world_runner_st runner= {
  (test_callback_runner_fn)_runner_default,
  (test_callback_runner_fn)_runner_default,
  (test_callback_runner_fn)_runner_default
};


void get_world(world_st *world)
{
  world->collections= collection;
  world->create= world_create;
  world->destroy= world_destroy;
  world->runner= &runner;
}