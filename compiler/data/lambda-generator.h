#pragma once

#include "compiler/data/lambda-class-data.h"

class LambdaGenerator {
public:
  LambdaGenerator(const std::string &name, const Location &location);

  LambdaGenerator &add_uses(std::vector<VertexPtr> uses, bool implicit_capture_this = false);
  LambdaGenerator &add_invoke_method(const VertexAdaptor<op_function> &function);
  LambdaGenerator &add_invoke_method_which_call_function(FunctionPtr called_function);
  LambdaGenerator &add_constructor_from_uses();

  LambdaPtr generate(FunctionPtr parent_function);
  LambdaPtr generate_and_require(FunctionPtr parent_function, DataStream<FunctionPtr> &os);

private:
  VertexAdaptor<op_func_name> create_name(const std::string &name);
  static LambdaPtr create_class(VertexAdaptor<op_func_name> name);
  VertexAdaptor<op_func_param_list> create_invoke_params(VertexAdaptor<op_function> function);
  VertexPtr create_invoke_cmd(VertexAdaptor<op_function> function);

  void add_this_to_captured_variables(VertexPtr &root);
  FunctionPtr register_invoke_method(VertexAdaptor<op_function> fun);
  LambdaGenerator &create_invoke_fun_returning_call(VertexAdaptor<op_func_call> &call_function, VertexAdaptor<op_func_param_list> invoke_params);

  static std::vector<VertexAdaptor<op_var>> get_params_as_vector_of_vars(FunctionPtr function, int shift = 0);

private:
  const Location created_location;
  LambdaPtr generated_lambda;
  std::vector<VertexPtr> uses;
};