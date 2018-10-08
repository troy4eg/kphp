#include "compiler/pipes/create-switch-foreach-vars.h"

#include "compiler/name-gen.h"

VertexPtr CreateSwitchForeachVarsF::process_switch(VertexPtr v) {
  VertexAdaptor<op_switch> switch_v = v;

  auto root_ss = VertexAdaptor<op_var>::create();
  root_ss->str_val = gen_unique_name("ss");
  root_ss->extra_type = op_ex_var_superlocal;
  root_ss->type_help = tp_string;
  switch_v->ss() = root_ss;

  auto root_ss_hash = VertexAdaptor<op_var>::create();
  root_ss_hash->str_val = gen_unique_name("ss_hash");
  root_ss_hash->extra_type = op_ex_var_superlocal;
  root_ss_hash->type_help = tp_int;
  switch_v->ss_hash() = root_ss_hash;

  auto root_switch_flag = VertexAdaptor<op_var>::create();
  root_switch_flag->str_val = gen_unique_name("switch_flag");
  root_switch_flag->extra_type = op_ex_var_superlocal;
  root_switch_flag->type_help = tp_bool;
  switch_v->switch_flag() = root_switch_flag;

  auto root_switch_var = VertexAdaptor<op_var>::create();
  root_switch_var->str_val = gen_unique_name("switch_var");
  root_switch_var->extra_type = op_ex_var_superlocal;
  root_switch_var->type_help = tp_var;
  switch_v->switch_var() = root_switch_var;
  return switch_v;
}

VertexPtr CreateSwitchForeachVarsF::process_foreach(VertexPtr v) {

  VertexAdaptor<op_foreach> foreach_v = v;
  VertexAdaptor<op_foreach_param> foreach_param = foreach_v->params();
  VertexAdaptor<op_var> x = foreach_param->x();
  //VertexPtr xs = foreach_param->xs();

  if (!x->ref_flag) {
    auto temp_var2 = VertexAdaptor<op_var>::create();
    temp_var2->str_val = gen_unique_name("tmp_expr");
    temp_var2->extra_type = op_ex_var_superlocal;
    temp_var2->needs_const_iterator_flag = true;
    foreach_param->temp_var() = temp_var2;

    foreach_v->params() = foreach_param;
  }
  return foreach_v;
}

VertexPtr CreateSwitchForeachVarsF::on_enter_vertex(VertexPtr v, LocalT *) {
  if (v->type() == op_switch) {
    return process_switch(v);
  }
  if (v->type() == op_foreach) {
    return process_foreach(v);
  }

  return v;
}