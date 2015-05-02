/*
 * Copyright 2003-2013 Jeffrey K. Hollingsworth
 *
 * This file is part of Active Harmony.
 *
 * Active Harmony is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Active Harmony is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Active Harmony.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * \file
 * \brief Harmony hsession structure manipulation functions.
 */

#ifndef __HSESSION_H__
#define __HSESSION_H__

#include "hsignature.h"
#include "hcfg.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef DOXYGEN_SKIP

typedef struct hsession {
    hsignature_t sig;
    hcfg_t cfg;
} hsession_t;
extern hsession_t HSESSION_INITIALIZER;

#endif

/**
 * \brief Initialize a new Harmony session descriptor.
 *
 * \param sess Session structure to initialize.
 *
 * \return Returns 0 on success, and -1 otherwise.
 */
int hsession_init(hsession_t* sess);

/**
 * \brief Fully copy a Harmony session descriptor.
 *
 * Performs a deep copy of a given Harmony session descriptor.
 *
 * \param[in,out] copy Destination of copy operation.
 * \param[in]     orig Source of copy operation.
 *
 * \return Returns 0 on success, and -1 otherwise.
 */
int hsession_copy(hsession_t* copy, const hsession_t* orig);

/**
 * \brief Release all resources associated with a Harmony session descriptor.
 *
 * \param sess Harmony descriptor initialized by hsession_init().
 */
void hsession_fini(hsession_t* sess);

#ifndef DOXYGEN_SKIP

int hsession_serialize(char** buf, int* buflen, const hsession_t* sess);
int hsession_deserialize(hsession_t* sess, char* buf);

#endif

#ifdef __cplusplus
}
#endif

#endif /* __HSESSION_H__ */
