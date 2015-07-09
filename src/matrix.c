/***************************************************************************/
/*                                                                         */
/* play.c - mruby testing                                                  */
/* Copyright (C) 2015 Paolo Bosetti and Matteo Ragni,                      */
/* paolo[dot]bosetti[at]unitn.it and matteo[dot]ragni[at]unitn.it          */
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

#include "matrix.h"

// Garbage collector handler, for play_data struct
// if play_data contains other dynamic data, free it too!
// Check it with GC.start
void matrix_destructor(mrb_state *mrb, void *p_) {
  gsl_matrix *v = (gsl_matrix *)p_;
  gsl_matrix_free(v);
  // or simply:
  // mrb_free(mrb, pd);
};

// Creating data type and reference for GC, in a const struct
const struct mrb_data_type matrix_data_type = {"matrix_data",
                                               matrix_destructor};

// Utility function for getting the struct out of the wrapping IV @data
void mrb_matrix_get_data(mrb_state *mrb, mrb_value self,
                                gsl_matrix **data) {
  mrb_value data_value;
  data_value = mrb_iv_get(mrb, self, mrb_intern_lit(mrb, "@data"));

  // Loading data from data_value into p_data:
  Data_Get_Struct(mrb, data_value, &matrix_data_type, *data);
  if (!*data)
    mrb_raise(mrb, E_RUNTIME_ERROR, "Could not access @data");
}

// Data Initializer C function (not exposed!)
static void mrb_matrix_init(mrb_state *mrb, mrb_value self, mrb_int n, mrb_int m) {
  mrb_value data_value; // this IV holds the data
  gsl_matrix *p_data;   // pointer to the C struct

  data_value = mrb_iv_get(mrb, self, mrb_intern_lit(mrb, "@data"));

  // if @data already exists, free its content:
  if (!mrb_nil_p(data_value)) {
    Data_Get_Struct(mrb, data_value, &matrix_data_type, p_data);
    free(p_data);
  }
  // Allocate and zero-out the data struct:
  p_data = gsl_matrix_calloc(n, m);
  if (!p_data)
    mrb_raise(mrb, E_RUNTIME_ERROR, "Could not allocate @data");

  // Wrap struct into @data:
  mrb_iv_set(
      mrb, self, mrb_intern_lit(mrb, "@data"), // set @data
      mrb_obj_value(                           // with value hold in struct
          Data_Wrap_Struct(mrb, mrb->object_class, &matrix_data_type, p_data)));
}

static mrb_value mrb_matrix_initialize(mrb_state *mrb, mrb_value self) {
  mrb_int n, m;
  mrb_get_args(mrb, "ii", &n, &m);

  // Call strcut initializer:
  mrb_matrix_init(mrb, self, n, m);
  return mrb_nil_value();
}



void mrb_gsl_matrix_init(mrb_state *mrb) {
  struct RClass *gsl;

  mrb_load_string(mrb, "class MatrixError < Exception; end");

  gsl = mrb_define_class(mrb, "Matrix", mrb->object_class);
  mrb_define_method(mrb, gsl, "initialize", mrb_matrix_initialize,
                    MRB_ARGS_NONE());
}

