/**
 * @file    main.c
 * @author  ECE485/585 Team 5
 *          members:
 *            Abdulaziz Alateeqi,
 *            Meshal Almutairi,
 *            Gene Hu,
 *            Eduardo Sanchez Simancas
 *
 * @brief   This program simulates a memory controller scheduler for a 12-core
 *          4.8 GHz processor employing a 16GB PC5-38400 DIMM
 * @version 0.1
 * @date    2023-11-07
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "common.h"
#include "dimm.h"
#include "memory_request.h"
#include "parser.h"
#include "queue.h"

/*** macro(s), enum(s), and struct(s) ***/
#define MAX_QUEUE_SIZE 16
#define DEFAULT_INPUT_FILE "trace.txt"
#define DEFAULT_OUTPUT_FILE "dram.txt"

/*** function prototype(s) ***/
void process_args(int argc, char *argv[], char **input_file, char **output_file, int *scheduling_policy);
void out_of_order(Queue_t *global_queue, MemoryRequest_t *current_request);
void advance_clock(uint64_t *clock_cycle, Queue_t *global_queue, Parser_t *parser);

/*** function(s) ***/
int main(int argc, char *argv[]) {
  clock_t begin_execution = clock();
  char *input_file_name, *output_file_name;
  int scheduling_policy = 0;  // default is level 0
  process_args(argc, argv, &input_file_name, &output_file_name, &scheduling_policy);

  printf("--- Simulation Parameters ---\n");
  printf("Scheduling Policy Level: %d\n", scheduling_policy);
  printf("Input File: %s\n", input_file_name);
  printf("Output File: %s\n", output_file_name);
  printf("-----------------------------\n");

  Parser_t *parser = parser_init(input_file_name);
  DIMM_t *PC5_38400 = NULL;
  Queue_t *global_queue = NULL;

  dimm_create(&PC5_38400, output_file_name);    // create DIMM
  queue_create(&global_queue, MAX_QUEUE_SIZE);  // create queue of size 16

  uint64_t clock_cycle = 0;  // tracking the clock cycle (CPU clock). DIMM clock cycle is 1/2.
  MemoryRequest_t *current_request = NULL;

  while (true) {
    if (current_request == NULL) {
      current_request = parser_next_request(parser, clock_cycle);  // only returns the request if the current cycle >= request's time
    }

    // DIMM clock cycle - only process request if there is one in the queue
    if (clock_cycle % 2 == 0 && !queue_is_empty(global_queue)) {
      process_request(&PC5_38400, &global_queue, clock_cycle, scheduling_policy);
      increment_aging_in_queue(global_queue);
    }

    // CPU clock cycle - enqueue if there is a request and queue is not full
    if (current_request != NULL && !queue_is_full(global_queue)) {
      if (scheduling_policy == LEVEL_3) {
        out_of_order(global_queue, current_request);

      } else {
        enqueue(&global_queue, *current_request);
      }
      log_memory_request("Enqueued:", current_request, clock_cycle);
      free(current_request);
      current_request = NULL;
    }

    if (parser->status == END_OF_FILE && queue_is_empty(global_queue)) {
      LOG("END OF SIMULATION\n");
      break;
    }

    advance_clock(&clock_cycle, global_queue, parser);
  }

  parser_destroy(parser);
  queue_destroy(&global_queue);
  dimm_destroy(&PC5_38400);
  clock_t end_execution = clock();
  printf("Total Clock Cycles: %" PRIu64 "\n", clock_cycle);
  printf("Program Execution Time: %lf seconds\n", (double)(end_execution - begin_execution) / CLOCKS_PER_SEC);
  return 0;
}

void advance_clock(uint64_t *clock_cycle, Queue_t *global_queue, Parser_t *parser) {
  if (queue_is_empty(global_queue) && parser->next_request->time > *clock_cycle) {
    LOG("No requests are processing. Advancing clock to next request time (%" PRIu64 ")\n", parser->next_request->time);
    *clock_cycle = parser->next_request->time;
  } else {
    *clock_cycle += 1;
  }
}

void process_args(int argc, char *argv[], char **input_file, char **output_file, int *scheduling_policy) {
  int opt;
  *input_file = DEFAULT_INPUT_FILE;
  *output_file = DEFAULT_OUTPUT_FILE;

  while ((opt = getopt(argc, argv, "i:o:s:h")) != -1) {
    switch (opt) {
      case 'i':  // Input file
        *input_file = optarg;
        break;
      case 'o':  // Output file
        *output_file = optarg;
        break;
      case 's':  // Scheduling policy
        *scheduling_policy = atoi(optarg);
        if (*scheduling_policy < 0 || *scheduling_policy > 3) {
          fprintf(stderr, "Invalid scheduling policy: %d. Must be between 0 and 3.\n", *scheduling_policy);
          exit(EXIT_FAILURE);
        }
        break;
      case 'h':
      case '?':
        fprintf(stderr, "Usage: %s [-i input_file] [-o output_file] [-s scheduling_policy]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
  }
}

void out_of_order(Queue_t *global_queue, MemoryRequest_t *current_request) {
  check_requests_age(global_queue);

  bool inserted = false;  // flag so we dont insert it twice

  // this if else is for reads>writes when valid
  if (current_request->operation == DATA_WRITE) {
    for (int i = 0; i < global_queue->size && !inserted; i++) {
      MemoryRequest_t *read_request = queue_peek_at(global_queue, i);
      if (read_request->operation != DATA_WRITE && read_request->bank_group == current_request->bank_group &&
          read_request->bank == current_request->bank && (read_request->row != current_request->row)) {
        // we put DATA_WRITE after the DATA_READ or IFETCH
        queue_insert_at(&global_queue, i + 1, *current_request);
        inserted = true;
        break;
      }
    }
  } else {
    for (int i = 0; i < global_queue->size && !inserted; i++) {
      MemoryRequest_t *write_request = queue_peek_at(global_queue, i);
      if (write_request->operation == DATA_WRITE && write_request->bank_group == current_request->bank_group &&
          write_request->bank == current_request->bank && (write_request->row != current_request->row)) {
        // we put the DATA_READ or IFETCH before the DATA_WRITE
        queue_insert_at(&global_queue, i, *current_request);
        inserted = true;
        break;
      }
    }
  }
  // if read > write is not valid, we want to prioritize hits.
  // put the read after write so we dont read stale data
  if (!inserted && current_request->operation != DATA_WRITE) {
    for (int i = 0; i < global_queue->size && !inserted; i++) {
      MemoryRequest_t *write_request = queue_peek_at(global_queue, i);
      if (write_request->operation == DATA_WRITE && write_request->bank_group == current_request->bank_group &&
          write_request->bank == current_request->bank && (write_request->row == current_request->row)) {
        queue_insert_at(&global_queue, i + 1, *current_request);
        inserted = true;
      }
    }
  }
  // if read > write is not valid, we want to prioritize hits.
  // put the read next to the other read
  if (!inserted && current_request->operation != DATA_WRITE) {
    for (int i = 0; i < global_queue->size && !inserted; i++) {
      MemoryRequest_t *read_request = queue_peek_at(global_queue, i);
      if (read_request->operation != DATA_WRITE && read_request->bank_group == current_request->bank_group &&
          read_request->bank == current_request->bank && (read_request->row == current_request->row)) {
        queue_insert_at(&global_queue, i + 1, *current_request);
        inserted = true;
        break;
      }
    }
  }

  // flag not up we do it normally
  if (!inserted) {
    enqueue(&global_queue, *current_request);
  }
}
