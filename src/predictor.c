//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
#include <string.h>
#include "predictor.h"

//
// TODO:Student Information
//
const char *studentName = "Brian Li";
const char *studentID   = "A14683659";
const char *email       = "brl088@ucsd.edu";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = { "Static", "Gshare",
                          "Tournament", "Custom" };

int ghistoryBits; // Number of bits used for Global History
int lhistoryBits; // Number of bits used for Local History
int pcIndexBits;  // Number of bits used for PC index
int bpType;       // Branch Prediction Type
int verbose;

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

void* predictor = 0;

int mask_value(uint32_t v, uint32_t bits) {
    uint32_t mask = (1 << bits) - 1;
    return v & mask;
}

void update_history(uint32_t* history, uint32_t bits, uint32_t outcome) {
    *history = mask_value((((*history) << 1) + outcome), bits);
}

//
//TODO: Add your own Branch Predictor data structures here
//
typedef struct gshare {
    uint8_t*  bht;
    uint32_t  bhr;
} gshare;

void init_gshare() {
    predictor = malloc(sizeof(gshare));
    ((gshare*) predictor)->bht = malloc((1 << ghistoryBits) * sizeof(uint8_t));
    ((gshare*) predictor)->bhr = 0;

    memset(((gshare*) predictor)->bht, WN, 1 << ghistoryBits);
}

int gshare_index(gshare* p, uint32_t pc) {
    return mask_value(pc ^ (p->bhr), ghistoryBits);
}

uint8_t predict_gshare(uint32_t pc) {
    gshare* p = (gshare*) predictor;
    int index = gshare_index(p, pc);
    return (p->bht)[index] <= 1 ? NOTTAKEN : TAKEN;
}

void train_gshare(uint32_t pc, uint8_t outcome) {
    gshare* p = (gshare*) predictor;
    int index = gshare_index(p, pc);
    if (outcome && (p->bht)[index] != ST) {
        (p->bht)[index] += 1;
    } else if(!outcome && (p->bht)[index] != SN) {
        (p->bht)[index] -= 1;
    }

    update_history(&(p->bhr), ghistoryBits, outcome);
}

typedef struct tournament {
    uint8_t*  gbht;
    uint8_t*  lbht;
    uint32_t*  lht;
    uint8_t*  chsr;
    uint32_t  gbhr;
} tournament;

void init_tournament() {
    predictor = malloc(sizeof(tournament));
    ((tournament*) predictor)->gbht = (uint8_t *) malloc((1 << ghistoryBits) * sizeof(uint8_t));
    ((tournament*) predictor)->lbht = (uint8_t *) malloc((1 << lhistoryBits) * sizeof(uint8_t));
    ((tournament*) predictor)->chsr = (uint8_t *) malloc((1 << ghistoryBits) * sizeof(uint8_t));
    ((tournament*) predictor)->lht  = (uint32_t *) calloc((1 << pcIndexBits), sizeof(uint32_t));
    ((tournament*) predictor)->gbhr = 0;

    memset(((tournament*) predictor)->gbht, WN, 1 << ghistoryBits);
    memset(((tournament*) predictor)->lbht, WN, 1 << lhistoryBits);
    memset(((tournament*) predictor)->chsr, WN, 1 << ghistoryBits);
}

uint8_t predict_tournament_local(uint32_t pc) {
    tournament* p = (tournament*) predictor;
    uint32_t l_history = (p->lht)[mask_value(pc, pcIndexBits)];
    return (p->lbht)[l_history] <= 1 ? NOTTAKEN : TAKEN;
}

uint8_t predict_tournament_global() {
    tournament* p = (tournament*) predictor;
    return (p->gbht)[p->gbhr] <= 1 ? NOTTAKEN : TAKEN;
}

uint8_t predict_tournament(uint32_t pc) {
    tournament* p = (tournament*) predictor;
    int idx = mask_value(pc, ghistoryBits);
    if ((p->chsr)[idx] <= 1) {
        return predict_tournament_local(pc);
    } else {
        return predict_tournament_global();
    }
}

void train_tournament_local(uint32_t pc, uint8_t outcome) {
    tournament* p = (tournament*) predictor;
    uint32_t* l_history = &(p->lht)[mask_value(pc, pcIndexBits)];

    if (outcome && (p->lbht)[*l_history] != ST) {
        (p->lbht)[*l_history] += 1;
    } else if (!outcome && (p->lbht)[*l_history] != SN) {
        (p->lbht)[*l_history] -= 1;
    }

    update_history(l_history, lhistoryBits, outcome);
}

void train_tournament_global(uint8_t outcome) {
    tournament* p = (tournament*) predictor;

    if (outcome && (p->gbht)[p->gbhr] != ST) {
        (p->gbht)[p->gbhr] += 1;
    } else if (!outcome && (p->gbht)[p->gbhr] != SN) {
        (p->gbht)[p->gbhr] -= 1;
    }

    update_history(&(p->gbhr), ghistoryBits, outcome);
}

void train_tournament(uint32_t pc, uint8_t outcome) {
    tournament* p = (tournament*) predictor;
    uint8_t local = predict_tournament_local(pc);
    uint8_t global = predict_tournament_global();
    int idx = mask_value(pc, ghistoryBits);
    uint8_t a = ~(local ^ outcome);
    uint8_t b = ~(global ^ outcome);
    int diff = ((int) a) - ((int) b);

    if (diff > 0 && (p->chsr)[idx] != SN) {
        (p->chsr)[idx] -= 1;
    } else if (diff < 0 && (p->chsr)[idx] != ST) {
        (p->chsr)[idx] += 1;
    }

    train_tournament_local(pc, outcome);
    train_tournament_global(outcome);
}


//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Initialize the predictor
//
void
init_predictor()
{
  //
  //TODO: Initialize Branch Predictor Data Structures
  //

  // Initialize the branch predictor based on the bpType
  switch (bpType) {
    case GSHARE:
      init_gshare();
      break;
    case TOURNAMENT:
      init_tournament();
      break;
    case CUSTOM:
    default:
      break;
  }

}

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint8_t
make_prediction(uint32_t pc)
{
  //
  //TODO: Implement prediction scheme
  //

  // Make a prediction based on the bpType
  switch (bpType) {
    case STATIC:
      return TAKEN;
    case GSHARE:
      return predict_gshare(pc);
    case TOURNAMENT:
      return predict_tournament(pc);
    case CUSTOM:
    default:
      break;
  }

  // If there is not a compatable bpType then return NOTTAKEN
  return NOTTAKEN;
}


// Train the predictor the last executed branch at PC 'pc' and with
// outcome 'outcome' (true indicates that the branch was taken, false
// indicates that the branch was not taken)
//
void
train_predictor(uint32_t pc, uint8_t outcome)
{
  //
  //TODO: Implement Predictor training
  //
  switch (bpType) {
    case GSHARE:
      train_gshare(pc, outcome);
      break;
    case TOURNAMENT:
      train_tournament(pc, outcome);
      break;
    case CUSTOM:
    default:
      break;
  }
}

