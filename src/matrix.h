/***************************************************************************/
/*                                                                         */
/* matrix.h - Matrix class for mruby                                       */
/* Copyright (C) 2015 Paolo Bosetti                                        */
/* paolo[dot]bosetti[at]unitn.it                                           */
/* Department of Industrial Engineering, University of Trento              */
/*                                                                         */
/* This library is free software.  You can redistribute it and/or          */
/* modify it under the terms of the GNU GENERAL PUBLIC LICENSE 2.0.        */
/*                                                                         */
/* This library is distributed in the hope that it will be useful,         */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of          */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           */
/* Artistic License 2.0 for more details.                                  */
/*                                                                         */
/* See the file LICENSE                                                    */
/*                                                                         */
/***************************************************************************/

#ifndef MATRIX_H
#define MATRIX_H

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>
#include <time.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_matrix.h>

#include "mruby.h"
#include "mruby/variable.h"
#include "mruby/string.h"
#include "mruby/data.h"
#include "mruby/class.h"
#include "mruby/value.h"
#include "mruby/array.h"
#include "mruby/numeric.h"
#include "mruby/compile.h"

#define E_MATRIX_ERROR (mrb_class_get(mrb, "MatrixError"))

/***********************************************\
 MATRICES
\***********************************************/

// Garbage collector handler, for play_data struct
// if play_data contains other dynamic data, free it too!
// Check it with GC.start
void matrix_destructor(mrb_state *mrb, void *p_);

// Utility function for getting the struct out of the wrapping IV @data
void mrb_matrix_get_data(mrb_state *mrb, mrb_value self, gsl_matrix **data);

void mrb_gsl_matrix_init(mrb_state *mrb);

#endif // MATRIX_H