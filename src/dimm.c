/**
 * @file  dram.c
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "dimm.h"

uint16_t timing_attribute[NUM_TIMING_CONSTRAINTS] = {
  TRC,
  TRAS,
  TRP,
  TRFC,
  TCWL,
  TCL,
  TRCD,
  TWR,
  TRTP,
  TBURST
};

uint8_t consecutive_cmd_attribute[NUM_CONSECUTIVE_CMD_CONSTRAINTS] = {
  TRRD_L,
  TRRD_S,
  TCCD_L,
  TCCD_S,
  TCCD_L_WR,
  TCCD_S_WR,
  TCCD_L_RTW,
  TCCD_S_RTW,
  TCCD_L_WTR,
  TCCD_S_WTR
};

/*** helper function(s) ***/
bool is_bank_active(DRAM_t *dram, MemoryRequest_t *request) {
  bool active_result = dram->bank_groups[request->bank_group].banks[request->bank].is_active;
  return active_result;
}

bool is_bank_precharged(DRAM_t *dram, MemoryRequest_t *request) {
  bool precharged_result = dram->bank_groups[request->bank_group].banks[request->bank].is_precharged;
  return precharged_result;
}

bool is_page_hit(DRAM_t *dram, MemoryRequest_t *request) {
  bool result = is_bank_active(dram, request) && dram->bank_groups[request->bank_group].banks[request->bank].active_row == request->row;
  return result;
}

bool is_page_miss(DRAM_t *dram, MemoryRequest_t *request) {
  bool result = is_bank_active(dram, request) && dram->bank_groups[request->bank_group].banks[request->bank].active_row != request->row;
  return result;
}

bool is_page_empty(DRAM_t *dram, MemoryRequest_t *request) {
  bool result = is_bank_precharged(dram, request) && !is_bank_active(dram, request);
  return result;
}

void activate_bank(DRAM_t *dram, MemoryRequest_t *request) {
  dram->bank_groups[request->bank_group].banks[request->bank].is_active = true;
  dram->bank_groups[request->bank_group].banks[request->bank].active_row = request->row;
}

void precharge_bank(DRAM_t *dram, MemoryRequest_t *request) {
  dram->bank_groups[request->bank_group].banks[request->bank].is_precharged = true;
  dram->bank_groups[request->bank_group].banks[request->bank].is_active = false;
}

char *issue_cmd(char *cmd, MemoryRequest_t *request, uint64_t cycle) {
  /**
   * @brief Constructs a command string to be written to the output file.
   *
   * @param cmd     command string (ACT, PRE, RD, or WR)
   * @param request memory request
   * @return char*  command string to be written to the output file
   */
  char *response = malloc(sizeof(char) * 100);
  char *temp = malloc(sizeof(char) * 100);

  sprintf(response, "%10" PRIu64 " %u %4s", cycle, request->channel, cmd);

  if (strncmp(cmd, "ACT", 3) == 0) {
    sprintf(temp, " %u %u 0x%04X", request->bank_group, request->bank, request->row);

  } else if (strncmp(cmd, "PRE", 3) == 0) {
    sprintf(temp, " %u %u", request->bank_group, request->bank);

  } else if (strncmp(cmd, "RD", 2) == 0 || strncmp(cmd, "WR", 2) == 0) {
    sprintf(temp, " %u %u 0x%04X", request->bank_group, request->bank, get_column(request));
  }

  strcat(response, temp);
  free(temp);

  return response;
}

void set_tfaw_timer(DRAM_t *dram) {
  for (int i = 0; i < NUM_TFAW_COUNTERS; i++) {
    if (dram->tFAW_timers[i] == 0) {
      dram->tFAW_timers[i] = TFAW;
      break;  // only want to set one counter at a time
    }
  }
}

void set_timing_constraint(DRAM_t *dram, MemoryRequest_t *request, TimingConstraints_t constraint_type) {
  dram->timing_constraints[request->bank_group][request->bank][constraint_type] = timing_attribute[constraint_type];
}

void set_consecutive_cmd_timers(DRAM_t *dram, ConsecutiveCmdConstraints_t constraint_type) {
  dram->consecutive_cmd_timers[constraint_type] = consecutive_cmd_attribute[constraint_type];
}

void set_trrd_timers(DRAM_t *dram) {
  dram->consecutive_cmd_timers[tRRD_L] = consecutive_cmd_attribute[tRRD_L];
  dram->consecutive_cmd_timers[tRRD_S] = consecutive_cmd_attribute[tRRD_S];
}

void set_tccd_timers(DRAM_t *dram) {
  dram->consecutive_cmd_timers[tCCD_L]     = consecutive_cmd_attribute[tCCD_L];
  dram->consecutive_cmd_timers[tCCD_S]     = consecutive_cmd_attribute[tCCD_S];
  dram->consecutive_cmd_timers[tCCD_L_WR]  = consecutive_cmd_attribute[tCCD_L_WR];
  dram->consecutive_cmd_timers[tCCD_S_WR]  = consecutive_cmd_attribute[tCCD_S_WR];
  dram->consecutive_cmd_timers[tCCD_L_RTW] = consecutive_cmd_attribute[tCCD_L_RTW];
  dram->consecutive_cmd_timers[tCCD_S_RTW] = consecutive_cmd_attribute[tCCD_S_RTW];
  dram->consecutive_cmd_timers[tCCD_L_WTR] = consecutive_cmd_attribute[tCCD_L_WTR];
  dram->consecutive_cmd_timers[tCCD_S_WTR] = consecutive_cmd_attribute[tCCD_S_WTR];
}

void decrement_tfaw_timers(DRAM_t *dram) {
  for (int i = 0; i < NUM_TFAW_COUNTERS; i++) {
    if (dram->tFAW_timers[i] != 0) {
      dram->tFAW_timers[i]--;
    }
  }
}

void decrement_timing_constraints(DRAM_t *dram) {
  // TODO: need a table to keep track of active timers. 3 nested loop is too big of a hit on performance
  // works for now
  for (int i = 0; i < NUM_BANK_GROUPS; i++) {
    for (int j = 0; j < NUM_BANKS_PER_GROUP; j++) {
      for (int k = 0; k < NUM_TIMING_CONSTRAINTS; k++) {
        if (dram->timing_constraints[i][j][k] != 0) {
          dram->timing_constraints[i][j][k]--;
        }
      }
    }
  }
}

void decrement_consecutive_cmd_timers(DRAM_t *dram) {
  for (int i = 0; i < NUM_CONSECUTIVE_CMD_CONSTRAINTS; i++) {
    if (dram->consecutive_cmd_timers[i] != 0) {
      dram->consecutive_cmd_timers[i]--;
    }
  }
}

bool is_timing_constraint_met(DRAM_t *dram, MemoryRequest_t *request, TimingConstraints_t constraint_type) {
  bool result = dram->timing_constraints[request->bank_group][request->bank][constraint_type] == 0;
  return result;
}

bool is_trrd_met(DRAM_t *dram, ConsecutiveCmdConstraints_t constraint_type) {
  return dram->consecutive_cmd_timers[constraint_type] == 0;
}

bool is_tccds_met(DRAM_t *dram, ConsecutiveCmdConstraints_t constraint_type) {
  return dram->consecutive_cmd_timers[constraint_type] == 0;
}

bool can_issue_act(DRAM_t *dram) {
  // if any counter is set to 0 then we can issue an ACT cmd
  // without violating the tFAW timing constraint
  for (int i = 0; i < NUM_TFAW_COUNTERS; i++) {
    if (dram->tFAW_timers[i] == 0) {
      return true;
    }
  }

  return false;
}

void check_requests_age(Queue_t *global_queue){
  if (global_queue == NULL || global_queue->list == NULL) {
    return; 
  }

  int old_request_age =-1;
  int young_request_age =-1;
  for (int i = 0; i < global_queue->size; i++) {
    MemoryRequest_t *request = queue_peek_at(global_queue, i);
    if (request != NULL) {
      if (request->aging >= TRC*8 && old_request_age == -1) {
        old_request_age = i;
      } 
      else if (request->aging < TRC && young_request_age == -1) {
        young_request_age = i;
      }

      if (old_request_age != -1 && young_request_age != -1) {
        break;
      }
    }
  }

  if (old_request_age != -1 && young_request_age != -1) {
    queue_insert_at(&global_queue, young_request_age,queue_delete_at(&global_queue,old_request_age));
  }

}

void increment_aging_in_queue(Queue_t *global_queue) {
    if (global_queue == NULL || global_queue->list == NULL) {
        return; 
    }

    for (uint8_t i = 0; i < global_queue->size; i++) {
        MemoryRequest_t *request = queue_peek_at(global_queue, i);
        if (request != NULL) {
            request->aging++; 
        }
    }
}

bool closed_page(DIMM_t **dimm, MemoryRequest_t *request, uint64_t clock) {
  DRAM_t *dram = &((*dimm)->channels[request->channel].DDR5_chip[0]);
  char *cmd = NULL;
  bool cmd_is_issued = false;

  if (request->state == PENDING) {
    request->state = ACT0;
  }

  // Process the request (one state per cycle)
  switch (request->state) {
    case ACT0:
      if (
        is_timing_constraint_met(dram, request, tRC) &&
        is_timing_constraint_met(dram, request, tRP)
      ) {
        cmd = issue_cmd("ACT0", request, clock);
        request->state = ACT1;
      }
      break;

    case ACT1:
      activate_bank(dram, request);

      cmd = issue_cmd("ACT1", request, clock);

      set_timing_constraint(dram, request, tRCD);
      set_timing_constraint(dram, request, tRAS);
      set_timing_constraint(dram, request, tRC);

      // next state
      if (request->operation == DATA_WRITE) {
        request->state = WR0;
      }
      else {
        request->state = RD0;
      }

      break;

    case RD0:
      if (is_timing_constraint_met(dram, request, tRCD)) {
        cmd = issue_cmd(request->operation == DATA_WRITE ? "WR0" : "RD0", request, clock);
        request->state = RD1;
      }
      break;

    case RD1:
      // issue cmd
      cmd = issue_cmd(request->operation == DATA_WRITE ? "WR1" : "RD1", request, clock);
      

      // set timers
      set_timing_constraint(dram, request, tCL);
      set_timing_constraint(dram, request, tRTP);

      // nest state
      request->state = PRE;
      break;

    case WR0:
      if (is_timing_constraint_met(dram, request, tRCD)) {
        cmd = issue_cmd(request->operation == DATA_WRITE ? "WR0" : "RD0", request, clock);
        request->state = WR1;
      }
      break;

    case WR1:
      // issue cmd
      cmd = issue_cmd(request->operation == DATA_WRITE ? "WR1" : "RD1", request, clock);

      // set timers
      set_timing_constraint(dram, request, tCWL);

      // nest state
      request->state = BUFFER;
      break;

    case PRE:
      // can precharge before bursting data
      if (request->operation == DATA_WRITE) {
        if (is_timing_constraint_met(dram, request, tWR) && is_timing_constraint_met(dram, request, tRAS)) {
          precharge_bank(dram, request);

          cmd = issue_cmd("PRE", request, clock);
          request->is_finished = true;

          set_timing_constraint(dram, request, tRP);

          request->state = COMPLETE;
        }
      }
      else {
        if (is_timing_constraint_met(dram, request, tRTP) && is_timing_constraint_met(dram, request, tRAS)) {
          precharge_bank(dram, request);

          cmd = issue_cmd("PRE", request, clock);
          request->is_finished = true;

          set_timing_constraint(dram, request, tRP);

          request->state = BUFFER;
        }
      }

      break;

    case BUFFER:
      // data is being stored in buffer while tCL is set
      if (request->operation == DATA_WRITE) {
        if (is_timing_constraint_met(dram, request, tCWL)) {
          set_timing_constraint(dram, request, tBURST);
          request->state = BURST;
        }
        
      }
      else {
        if (is_timing_constraint_met(dram, request, tCL)) {
          set_timing_constraint(dram, request, tBURST);
          request->state = BURST;
        }
      }
      break;

    case BURST:
      if (is_timing_constraint_met(dram, request, tBURST)) {
        if (request->operation == DATA_WRITE) {
          set_timing_constraint(dram, request, tWR);
          request->state = PRE;
        }
        else {
          request->state = COMPLETE;
        }
        
      }
      break;

    case COMPLETE:
      break;

    case PENDING:
    default:
      fprintf(stderr, "Error: Unknown state encountered\n");
      exit(EXIT_FAILURE);
  }

  // writing commands to output file
  if (cmd != NULL) {
    fprintf((*dimm)->output_file, "%s\n", cmd);
    free(cmd);
    cmd_is_issued = true;
  }

  return cmd_is_issued;
}

bool open_page(DIMM_t **dimm, MemoryRequest_t *request, uint64_t cycle) {
  DRAM_t *dram = &((*dimm)->channels[request->channel].DDR5_chip[0]);
  char *cmd = NULL;
  bool cmd_is_issued = false;

  // Set the initial state before processing the request
  if (request->state == PENDING) {
    if (is_page_hit(dram, request)) {
      if (request->operation == DATA_WRITE) {
        request->state = WR0;
      }
      else {
        request->state = RD0;
      }
      dram->bank_groups[request->bank_group].banks[request->bank].last_request_operation = request->operation;
    }
    else if (is_page_miss(dram, request)) {
      request->state = PRE;
    }
    else if (is_page_empty(dram, request)) {
      if (!can_issue_act(dram)) {
        return cmd_is_issued;
      }

      request->state = ACT0;
      dram->bank_groups[request->bank_group].banks[request->bank].last_request_operation = request->operation;
    }
    else {
      fprintf(stderr, "Error: Unknown page state encountered\n");
      exit(EXIT_FAILURE);
    }
  }

  // Process the request (one state per cycle)
  switch (request->state) {
    case PRE:
      if (dram->bank_groups[request->bank_group].banks[request->bank].last_request_operation == DATA_WRITE) {
        if (
          is_timing_constraint_met(dram, request, tRAS) &&
          is_timing_constraint_met(dram, request, tCWL) &&
          is_timing_constraint_met(dram, request, tBURST) &&
          is_timing_constraint_met(dram, request, tWR) &&
          is_timing_constraint_met(dram, request, tRP) 
        ) {
          precharge_bank(dram, request);

          // issue cmd
          cmd = issue_cmd("PRE", request, cycle);
          dram->last_interface_cmd = PRECHARGE;
          dram->last_bank_group = request->bank_group;
          dram->bank_groups[request->bank_group].banks[request->bank].in_progress = true;

          // set timers
          set_timing_constraint(dram, request, tRP);

          // next state
          request->state = ACT0;
        }
      }
      else {
        if (
          is_timing_constraint_met(dram, request, tRAS) &&
          is_timing_constraint_met(dram, request, tRTP) &&
          is_timing_constraint_met(dram, request, tRP)
        ) {
          precharge_bank(dram, request);

          // issue cmd
          cmd = issue_cmd("PRE", request, cycle);
          dram->last_interface_cmd = PRECHARGE;
          dram->last_bank_group = request->bank_group;
          dram->bank_groups[request->bank_group].banks[request->bank].in_progress = true;

          // set timers
          set_timing_constraint(dram, request, tRP);

          // next state
          request->state = ACT0;
        }
      }

      break;

    case ACT0:
      if (!can_issue_act(dram)) {
        return cmd_is_issued;
      }

      if (dram->last_interface_cmd == ACTIVATE) {
        if (dram->last_bank_group == request->bank_group) {
          if (
            is_timing_constraint_met(dram, request, tRC) &&
            is_timing_constraint_met(dram, request, tRP) &&
            is_trrd_met(dram, tRRD_L)
          ) {
            cmd = issue_cmd("ACT0", request, cycle);
            request->state = ACT1;
            dram->bank_groups[request->bank_group].banks[request->bank].in_progress = true;
          }
        }
        else {
          if (
            is_timing_constraint_met(dram, request, tRC) &&
            is_timing_constraint_met(dram, request, tRP) &&
            is_trrd_met(dram, tRRD_S)
          ) {
            cmd = issue_cmd("ACT0", request, cycle);
            request->state = ACT1;
            dram->bank_groups[request->bank_group].banks[request->bank].in_progress = true;
          }
        }
      }
      else {
        if (
          is_timing_constraint_met(dram, request, tRC) &&
          is_timing_constraint_met(dram, request, tRP)
        ) {
          cmd = issue_cmd("ACT0", request, cycle);
          request->state = ACT1;
          dram->bank_groups[request->bank_group].banks[request->bank].in_progress = true;
        }
      }

      break;

    case ACT1:
      activate_bank(dram, request);

      // issue cmd
      cmd = issue_cmd("ACT1", request, cycle);
      dram->last_interface_cmd = ACTIVATE;
      dram->last_bank_group = request->bank_group;

      // set timers
      set_timing_constraint(dram, request, tRCD);
      set_timing_constraint(dram, request, tRAS);
      set_timing_constraint(dram, request, tRC);
      set_trrd_timers(dram);
      set_tfaw_timer(dram);

      // next state
      if (request->operation == DATA_WRITE) {
        request->state = WR0;
      }
      else {
        request->state = RD0;
      }

      break;

    case RD0:
      if (dram->last_interface_cmd == WRITE) {
        if (dram->last_bank_group == request->bank_group) {
          if (
            is_timing_constraint_met(dram, request, tRCD) &&
            is_tccds_met(dram, tCCD_L_WTR)
          ) {
            cmd = issue_cmd(request->operation == DATA_WRITE ? "WR0" : "RD0", request, cycle);
            request->state = RD1;
            dram->bank_groups[request->bank_group].banks[request->bank].in_progress = true;
          }
        }
        else {
          
          if (
            is_timing_constraint_met(dram, request, tRCD) &&
            is_tccds_met(dram, tCCD_S_WTR)
          ) {
            cmd = issue_cmd(request->operation == DATA_WRITE ? "WR0" : "RD0", request, cycle);
            request->state = RD1;
            dram->bank_groups[request->bank_group].banks[request->bank].in_progress = true;
          }
        }

      }
      else if (dram->last_interface_cmd == READ) {
        if (dram->last_bank_group == request->bank_group) {
          if (
            is_timing_constraint_met(dram, request, tRCD) &&
            is_tccds_met(dram, tCCD_L)
          ) {
            cmd = issue_cmd(request->operation == DATA_WRITE ? "WR0" : "RD0", request, cycle);
            request->state = RD1;
            dram->bank_groups[request->bank_group].banks[request->bank].in_progress = true;
          }
        }
        else {
          if (
            is_timing_constraint_met(dram, request, tRCD) &&
            is_tccds_met(dram, tCCD_S)
          ) {
            cmd = issue_cmd(request->operation == DATA_WRITE ? "WR0" : "RD0", request, cycle);
            request->state = RD1;
            dram->bank_groups[request->bank_group].banks[request->bank].in_progress = true;
          }
        }
      }
      else {
        if (is_timing_constraint_met(dram, request, tRCD)) {
          cmd = issue_cmd(request->operation == DATA_WRITE ? "WR0" : "RD0", request, cycle);
          request->state = RD1;
          dram->bank_groups[request->bank_group].banks[request->bank].in_progress = true;
        }
      }

      break;

    case RD1:
      // issue cmd
      cmd = issue_cmd(request->operation == DATA_WRITE ? "WR1" : "RD1", request, cycle);
      request->is_finished = true;
      dram->last_interface_cmd = READ;
      dram->last_bank_group = request->bank_group;

      // set timers
      set_timing_constraint(dram, request, tCL);
      set_timing_constraint(dram, request, tRTP);
      set_tccd_timers(dram);

      // nest state
      request->state = BUFFER;
      break;

    case WR0:
      if (dram->last_interface_cmd == WRITE) {
        if (dram->last_bank_group == request->bank_group) {
          if (
            is_timing_constraint_met(dram, request, tRCD) &&
            is_tccds_met(dram, tCCD_L_WR)
          ) {
            cmd = issue_cmd(request->operation == DATA_WRITE ? "WR0" : "RD0", request, cycle);
            request->state = WR1;
            dram->bank_groups[request->bank_group].banks[request->bank].in_progress = true;
          }
        }
        else {
          if (
            is_timing_constraint_met(dram, request, tRCD) &&
            is_tccds_met(dram, tCCD_S_WR)
          ) {
            cmd = issue_cmd(request->operation == DATA_WRITE ? "WR0" : "RD0", request, cycle);
            request->state = WR1;
            dram->bank_groups[request->bank_group].banks[request->bank].in_progress = true;
          }
        }
      }
      else if (dram->last_interface_cmd == READ) {
        if (dram->last_bank_group == request->bank_group) {
          if (
            is_timing_constraint_met(dram, request, tRCD) &&
            is_tccds_met(dram, tCCD_L_RTW)
          ) {
            cmd = issue_cmd(request->operation == DATA_WRITE ? "WR0" : "RD0", request, cycle);
            request->state = WR1;
            dram->bank_groups[request->bank_group].banks[request->bank].in_progress = true;
          }
        }
        else {
          if (
            is_timing_constraint_met(dram, request, tRCD) &&
            is_tccds_met(dram, tCCD_S_RTW)
          ) {
            cmd = issue_cmd(request->operation == DATA_WRITE ? "WR0" : "RD0", request, cycle);
            request->state = WR1;
            dram->bank_groups[request->bank_group].banks[request->bank].in_progress = true;
          }
        }
      }
      else {
        if (is_timing_constraint_met(dram, request, tRCD)) {
          cmd = issue_cmd(request->operation == DATA_WRITE ? "WR0" : "RD0", request, cycle);
          request->state = WR1;
          dram->bank_groups[request->bank_group].banks[request->bank].in_progress = true;
        }
      }
      break;

    case WR1:
      // issue cmd
      cmd = issue_cmd(request->operation == DATA_WRITE ? "WR1" : "RD1", request, cycle);
      request->is_finished = true;
      dram->last_interface_cmd = WRITE;
      dram->last_bank_group = request->bank_group;

      // set timers
      set_timing_constraint(dram, request, tCWL);
      set_tccd_timers(dram);

      // nest state
      request->state = BUFFER;
      break;

    case BUFFER:
      if (request->operation == DATA_WRITE) {
        if (is_timing_constraint_met(dram, request, tCWL)) {
          set_timing_constraint(dram, request, tBURST);
          request->state = BURST;
        }
        
      }
      else {
        if (is_timing_constraint_met(dram, request, tCL)) {
          set_timing_constraint(dram, request, tBURST);
          request->state = BURST;
        }
      }
      break;

    case BURST:
      if (is_timing_constraint_met(dram, request, tBURST)) {
        if (request->operation == DATA_WRITE) {
          set_timing_constraint(dram, request, tWR);
        }
        request->state = COMPLETE;
        dram->bank_groups[request->bank_group].banks[request->bank].in_progress = false;
      }
      break;

    case COMPLETE:
      break;

    case PENDING:
    default:
      fprintf(stderr, "Error: Unknown state encountered\n");
      exit(EXIT_FAILURE);
  }

  // writing commands to output file
  if (cmd != NULL) {
    fprintf((*dimm)->output_file, "%s\n", cmd);
    free(cmd);
    cmd_is_issued = true;
  }

  return cmd_is_issued;
}

void level_zero_algorithm(DIMM_t **dimm, Queue_t **q, uint64_t clock) {
  MemoryRequest_t *request = queue_peek(*q);
  DRAM_t *dram = &((*dimm)->channels[request->channel].DDR5_chip[0]);

  if ((*q)->size > 1) {  
    MemoryRequest_t *next_request = queue_peek_at(*q, 1);
    if (!request->is_finished) {
      closed_page(dimm, request, clock);
    }
    else {
      closed_page(dimm, request, clock);
      closed_page(dimm, next_request, clock);
    }
  }
  else {
    if (request->state != COMPLETE) {
      closed_page(dimm, request, clock);
    }
  }

  if (request && request->state == COMPLETE) {
    log_memory_request("Dequeued:", request, clock);
    dequeue(q);
  }

  decrement_timing_constraints(dram);
}

void level_one_algorithm(DIMM_t **dimm, Queue_t **q, uint64_t clock) {
  MemoryRequest_t *request = queue_peek(*q);
  DRAM_t *dram = &((*dimm)->channels[request->channel].DDR5_chip[0]);

  // if current request is not finish, finish it
  if (!request->is_finished) {
    open_page(dimm, request, clock);
  }
  // else if current request is finish, start next request
  else {
    for (int i = 0; i < (*q)->size; i++) {
      MemoryRequest_t *next_request = queue_peek_at(*q, i);
      open_page(dimm, next_request, clock);

      if (!next_request->is_finished) {
        break;
      }
    }
  }

  // if current request is ready to be dequeue, delete it
  if (request && request->state == COMPLETE) {
    log_memory_request("Dequeued:", request, clock);
    dequeue(q);
  }
  
  decrement_tfaw_timers(dram);
  decrement_timing_constraints(dram);
  decrement_consecutive_cmd_timers(dram);
}

void bank_level_parallelism(DIMM_t **dimm, Queue_t **q, uint64_t clock) {
  bool is_cmd_issued = false;
  DRAM_t *dram = &((*dimm)->channels[0].DDR5_chip[0]);

  for (int index = 0; index < (*q)->size; index++) {
    MemoryRequest_t *request = queue_peek_at(*q, index);

    // delete once done
    if (request->state == COMPLETE) {
      queue_delete_at(q, index);
      index--; // decrement index to account for the deleted element
      continue;
    }

    if (request->is_finished) {
      open_page(dimm, request, clock);
      continue;
    }

    if (index != 0) {

      MemoryRequest_t *last_request = queue_peek_at(*q, index - 1);
      if (
        !last_request->is_finished &&
        last_request->bank_group == request->bank_group &&
        last_request->bank == request->bank
      ) {
        continue;
      }

    }

    is_cmd_issued = open_page(dimm, request, clock);

    if (is_cmd_issued) {
      break;
    }
  }

  decrement_timing_constraints(dram);
  decrement_consecutive_cmd_timers(dram);
  decrement_tfaw_timers(dram);
}

void dram_init(DRAM_t *dram) {
  // Initialize the DRAM with all banks precharged
  for (int i = 0; i < NUM_BANK_GROUPS; i++) {
    for (int j = 0; j < NUM_BANKS_PER_GROUP; j++) {
      dram->bank_groups[i].banks[j].is_precharged = true;
      dram->bank_groups[i].banks[j].is_active = false;
      dram->bank_groups[i].banks[j].active_row = 0;
      dram->bank_groups[i].banks[j].in_progress = false;

      // zero out bank timers
      for (int k = 0; k < NUM_TIMING_CONSTRAINTS; k++) {
        dram->timing_constraints[i][j][k] = 0;
      }
    }
  }

  // zero out dram timers
  for (int i = 0; i < NUM_CONSECUTIVE_CMD_CONSTRAINTS; i++) {
    dram->consecutive_cmd_timers[i] = 0;
  }

  // zero out tfaw timers
  for (int i = 0; i < NUM_TFAW_COUNTERS; i++) {
    dram->tFAW_timers[i] = 0;
  }
}

/*** function(s) ***/
void dimm_create(DIMM_t **dimm, char *output_file_name) {
  *dimm = malloc(sizeof(DIMM_t));

  if (*dimm == NULL) {
    fprintf(stderr, "%s:%d: malloc failed\n", __FILE__, __LINE__);
    exit(EXIT_FAILURE);
  }

  // opening the file
  (*dimm)->output_file = fopen(output_file_name, "w");
  if ((*dimm)->output_file == NULL) {
    fprintf(stderr, "%s:%d: fopen failed\n", __FILE__, __LINE__);
    exit(EXIT_FAILURE);
  }

  for (int i = 0; i < NUM_CHANNELS; i++) {
    for (int j = 0; j < NUM_CHIPS_PER_CHANNEL; j++) {
      dram_init(&((*dimm)->channels[i].DDR5_chip[j]));
    }
  }
}

void dimm_destroy(DIMM_t **dimm) {
  if (*dimm != NULL) {
    // closing the file
    if ((*dimm)->output_file) {
      fclose((*dimm)->output_file);
    }

    free(*dimm);
    *dimm = NULL;  // remove dangler
  }
}

void process_request(DIMM_t **dimm, Queue_t **q, uint64_t clock, uint8_t scheduling_algorithm) {
  switch (scheduling_algorithm) {
    case LEVEL_0:
      level_zero_algorithm(dimm, q, clock);
      break;

    case LEVEL_1:
      level_one_algorithm(dimm, q, clock);
      break;

    case LEVEL_2:
    case LEVEL_3:
      bank_level_parallelism(dimm, q, clock);
      break;

    default:
      break;
  }
}
