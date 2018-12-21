#include "compiler/pipes/clone-parent-method-with-context.h"

#include "compiler/data/function-data.h"
#include "compiler/data/class-data.h"
#include "compiler/data/src-file.h"
#include "compiler/name-gen.h"
#include "compiler/gentree.h"
#include "compiler/compiler-core.h"

/**
 * Через этот pass проходят функции вида
 * baseclassname$$fname$$contextclassname
 * которые получились клонированием родительского дерева
 * В будущем этот pass должен уйти, когда лямбды будут вычленяться не на уровне gentree, а позже
 */
class PatchInheritedMethodPass : public FunctionPassBase {
  DataStream<FunctionPtr> &function_stream;
public:
  explicit PatchInheritedMethodPass(DataStream<FunctionPtr> &function_stream) :
    function_stream(function_stream) {}

  VertexPtr on_enter_vertex(VertexPtr root, LocalT *) {
    if (root->type() == op_constructor_call && root->get_func_id() && root->get_func_id()->is_lambda()) {
      ClassPtr lambda_class = root->get_func_id()->class_id;
      FunctionPtr invoke_method = lambda_class->members.get_instance_method("__invoke")->function;
      vector<VertexPtr> uses_of_lambda;

      return GenTree::generate_anonymous_class(invoke_method->root, function_stream, current_function, std::move(uses_of_lambda), current_function->file_id);
    }

    return root;
  }
};

void CloneParentMethodWithContextF::create_ast_of_function_with_context(FunctionPtr function, DataStream<FunctionPtr> &os) {
  // функция имеет название baseclassname$$fname$$contextclassname
  // нужно достать функцию baseclassname$$fname и клонировать её дерево
  std::string full_name = function->root->name()->get_string();
  auto pos = full_name.rfind("$$");

  FunctionPtr parent_f = G->get_function(full_name.substr(0, pos));

  function->root = clone_vertex(parent_f->root);
  function->root->name()->set_string(full_name);
  function->root->set_func_id(function);

  PatchInheritedMethodPass pass(os);
  run_function_pass(function, &pass);
}

void CloneParentMethodWithContextF::execute(FunctionPtr function, DataStream <FunctionPtr> &os) {
  AUTO_PROF (clone_method_with_context);

  // обрабатываем функции для статического наследования (контекстные) — вида baseclassname$$fname$$contextclassname
  // сюда прокидываются такие функции из collect required — этот пайп сделан специально,
  // чтобы работать в несколько потоков
  // причём это нужно до gen tree postprocess, т.к. родительские могут быть не required (и не прошли пайпы)
  if (function->context_class && function->context_class != function->class_id) {
    create_ast_of_function_with_context(function, os);
  }

  os << function;
}