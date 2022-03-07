#ifndef CACHE_OP_IDS_H
#define CACHE_OP_IDS_H

#include <stdint.h>

/**
 * Operation ID's such that the functions in bddmc and lddmc can make use of 
 * Sylvan's operation cache.
 */
static const uint64_t CACHE_BDD_SAT             = (200LL<<40);
static const uint64_t CACHE_LDD_SAT             = (201LL<<40);
static const uint64_t CACHE_BDD_REACH           = (300LL<<40);
static const uint64_t CACHE_BDD_REACH_PARTIAL   = (301LL<<40);
static const uint64_t CACHE_LDD_REACH           = (302LL<<40);
static const uint64_t CACHE_LDD_IMAGE           = (303LL<<40);
static const uint64_t CACHE_LDD_EXTEND_REL      = (304LL<<40);

#endif
