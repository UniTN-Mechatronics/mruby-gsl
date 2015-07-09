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

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <vector>
#include <stdlib.h>
#include <sys/param.h>
#include <time.h>
#include <gsl/gsl_vector.h>

#include "mruby.h"
#include "mruby/variable.h"
#include "mruby/string.h"
#include "mruby/data.h"
#include "mruby/class.h"
#include "mruby/value.h"
#include "mruby/array.h"
#include "mruby/numeric.h"
#include "mruby/compile.h"



// Struct holding data:
typedef struct {
  double d;
  int i;
  std::vector<int> first;
  double *ary;
} play_data_s;

// Garbage collector handler, for play_data struct
// if play_data contains other dynamic data, free it too!
// Check it with GC.start
static void play_data_destructor(mrb_state *mrb, void *p_) {
  play_data_s *pd = (play_data_s *)p_;
  free(pd->ary);
  free(pd);
  // or simply:
  // mrb_free(mrb, pd);
};

// Creating data type and reference for GC, in a const struct
const struct mrb_data_type play_data_type = {"play_data", play_data_destructor};

// Utility function for getting the struct out of the wrapping IV @data
static void mrb_play_get_data(mrb_state *mrb, mrb_value self,
                              play_data_s **data) {
  mrb_value data_value;
  data_value = mrb_iv_get(mrb, self, mrb_intern_lit(mrb, "@data"));

  // Loading data from data_value into p_data:
  Data_Get_Struct(mrb, data_value, &play_data_type, *data);
  if (!*data)
    mrb_raise(mrb, E_RUNTIME_ERROR, "Could not access @data");
}

// Data Initializer C function (not exposed!)
static void mrb_play_init(mrb_state *mrb, mrb_value self, double d) {
  mrb_value data_value; // this IV holds the data
  play_data_s *p_data;  // pointer to the C struct

  data_value = mrb_iv_get(mrb, self, mrb_intern_lit(mrb, "@data"));

  // if @data already exists, free its content:
  if (!mrb_nil_p(data_value)) {
    Data_Get_Struct(mrb, data_value, &play_data_type, p_data);
    free(p_data);
  }
  // Allocate and zero-out the data struct:
  p_data = (play_data_s *)malloc(sizeof(play_data_s));
  memset(p_data, 0, sizeof(play_data_s));
  if (!p_data)
    mrb_raise(mrb, E_RUNTIME_ERROR, "Could not allocate @data");

  // Wrap struct into @data:
  mrb_iv_set(
      mrb, self, mrb_intern_lit(mrb, "@data"), // set @data
      mrb_obj_value(                           // with value hold in struct
          Data_Wrap_Struct(mrb, mrb->object_class, &play_data_type, p_data)));

  // Now set values into struct:
  p_data->d = d;
  p_data->i = 10;
  p_data->ary = (double *)malloc(sizeof(double) * p_data->i);
  memset(p_data->ary, 0, sizeof(double) * p_data->i);
}

static mrb_value mrb_play_initialize(mrb_state *mrb, mrb_value self) {
  mrb_value d = mrb_nil_value();
  mrb_get_args(mrb, "o", &d);

  // Call strcut initializer:
  mrb_play_init(mrb, self, mrb_to_flo(mrb, d));
  return mrb_nil_value();
}

static mrb_value mrb_play_d(mrb_state *mrb, mrb_value self) {
  play_data_s *p_data = NULL;

  // call utility for unwrapping @data into p_data:
  mrb_play_get_data(mrb, self, &p_data);

  // Play with p_data content:
  return mrb_float_value(mrb, p_data->d);
}

static mrb_value mrb_play_set_d(mrb_state *mrb, mrb_value self) {
  mrb_value d_value = mrb_nil_value();
  play_data_s *p_data = NULL;

  mrb_get_args(mrb, "o", &d_value);

  // call utility for unwrapping @data into p_data:
  mrb_play_get_data(mrb, self, &p_data);

  p_data->d = mrb_to_flo(mrb, d_value);
  return d_value;
}

static mrb_value mrb_play_ary(mrb_state *mrb, mrb_value self) {
  play_data_s *p_data = NULL;
  mrb_value ary;
  mrb_int i;
  // call utility for unwrapping @data into p_data:
  mrb_play_get_data(mrb, self, &p_data);

  // Play with p_data content:
  ary = mrb_ary_new_capa(mrb, p_data->i);
  for (i = 0; i < p_data->i; i++) {
    mrb_ary_set(mrb, ary, i, mrb_float_value(mrb, p_data->ary[i]));
  }
  return ary;
}

static mrb_value mrb_play_set_ary(mrb_state *mrb, mrb_value self) {
  mrb_value ary_in = mrb_nil_value();
  play_data_s *p_data = NULL;
  mrb_int i;
  mrb_value elem;
  mrb_get_args(mrb, "A", &ary_in);

  // call utility for unwrapping @data into p_data:
  mrb_play_get_data(mrb, self, &p_data);
  if (p_data->ary)
    free(p_data->ary);

  p_data->i = RARRAY_LEN(ary_in);
  p_data->ary = (double *)malloc(sizeof(double) * p_data->i);
  for (i = 0; i < p_data->i; i++) {
    elem = mrb_ary_entry(ary_in, i);
    if (mrb_fixnum_p(elem) || mrb_float_p(elem))
      p_data->ary[i] = mrb_to_flo(mrb, elem);
    else {
      p_data->i = 0;
      p_data->ary = (double *)malloc(0);
      mrb_raisef(mrb, E_RUNTIME_ERROR, "Non-numeric entry at position %S",
                 mrb_fixnum_value(i));
    }
  }
  return mrb_fixnum_value(i);
}

void mrb_mruby_gsl_gem_init(mrb_state *mrb) {
  struct RClass *gsl;

  mrb_load_string(mrb,
                  "class SleepError < Exception; attr_reader :actual; end");

  play = mrb_define_class(mrb, "Play", mrb->object_class);
  mrb_define_method(mrb, play, "check", mrb_play_check, MRB_ARGS_NONE());
  mrb_define_method(mrb, play, "initialize", mrb_play_initialize,
                    MRB_ARGS_NONE());
  mrb_define_method(mrb, play, "d", mrb_play_d, MRB_ARGS_NONE());
  mrb_define_method(mrb, play, "d=", mrb_play_set_d, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, play, "ary", mrb_play_ary, MRB_ARGS_NONE());
  mrb_define_method(mrb, play, "ary=", mrb_play_set_ary, MRB_ARGS_REQ(1));

}

void mrb_mruby_gsl_gem_final(mrb_state *mrb) {}

