/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#include "basic/mir.hpp"
#include "compiler/ffi.hpp"
#include "compiler/type_infer_visitor.hpp"

namespace mimium {

using optvalptr = std::optional<mir::valueptr>;
class MirGenerator {
 public:
  explicit MirGenerator(TypeEnv& typeenv) : typeenv(typeenv) {}
  struct ExprKnormVisitor : public VisitorBase<mir::valueptr&> {
    explicit ExprKnormVisitor(MirGenerator& parent, mir::blockptr block,
                              const std::optional<mir::valueptr>& fnctx)
        : mirgen(parent), block(block), fnctx(fnctx) {}
    mir::valueptr operator()(ast::Op& ast);
    mir::valueptr operator()(ast::Number& ast);
    mir::valueptr operator()(ast::String& ast);
    mir::valueptr operator()(ast::Symbol& ast);
    mir::valueptr operator()(ast::Self& ast);
    mir::valueptr operator()(ast::Lambda& ast);
    mir::valueptr operator()(ast::Fcall& ast);
    mir::valueptr operator()(ast::Struct& ast);
    mir::valueptr operator()(ast::StructAccess& ast);
    mir::valueptr operator()(ast::ArrayInit& ast);
    mir::valueptr operator()(ast::ArrayAccess& ast);
    mir::valueptr operator()(ast::Tuple& ast);
    mir::valueptr operator()(ast::If& ast);
    mir::valueptr operator()(ast::Block& ast);
    mir::valueptr genInst(ast::ExprPtr expr,
                          std::optional<std::string> const& lvar = std::nullopt) {
      lvar_holder = lvar;
      auto res = std::visit(*this, *expr);
      lvar_holder = std::nullopt;
      return res;
    }

    MirGenerator& mirgen;
    mir::valueptr emplace(mir::Instructions&& inst);
    mir::valueptr genAllocate(std::string const& name, types::Value const& type);
    mir::valueptr genFcallInst(ast::Fcall& fcall, optvalptr const& when);

    const std::optional<mir::valueptr>& fnctx;
   private:
    std::pair<optvalptr, mir::blockptr> genIfBlock(ast::ExprPtr& block, std::string const& label);

    mir::blockptr block;
    std::optional<std::string> lvar_holder;
  };
  struct AssignKnormVisitor {
    explicit AssignKnormVisitor(MirGenerator& parent, ExprKnormVisitor& exprvisitor,
                                ast::ExprPtr expr)
        : mirgen(parent), exprvisitor(exprvisitor), expr(std::move(expr)){};
    void operator()(ast::DeclVar& ast);
    void operator()(ast::ArrayLvar& ast);
    void operator()(ast::TupleLvar& ast);

   private:
    MirGenerator& mirgen;
    ExprKnormVisitor& exprvisitor;

    ast::ExprPtr expr;
  };
  struct StatementKnormVisitor {
    explicit StatementKnormVisitor(ExprKnormVisitor& evisitor)
        : exprvisitor(evisitor), mirgen(evisitor.mirgen), retvalue(std::nullopt),fnctx(exprvisitor.fnctx) {}
        
    void operator()(ast::Assign& ast);
    void operator()(ast::Fdef& ast);
    void operator()(ast::Return& ast);
    void operator()(ast::Time& ast);
    void operator()(ast::Fcall& ast);
    void operator()(ast::For& ast);
    void operator()(ast::If& ast);
    void genInst(ast::Statement& stmt) { return std::visit(*this, stmt); }

    optvalptr retvalue;

   private:
    ExprKnormVisitor& exprvisitor;
    MirGenerator& mirgen;
    const std::optional<mir::valueptr>& fnctx;
  };
  mir::blockptr generate(ast::Statements& topast);

 private:
  std::pair<optvalptr, mir::blockptr> generateBlock(ast::Block& block, std::string label,
                                                    optvalptr const& fnctx);

  // expect return value
  // auto emplaceExpr(mir::Instructions&& inst) { return require(emplace(std::move(inst))); }
  template <typename FROM, typename TO, class LAMBDA>
  auto transformArgs(FROM&& from, TO&& to, LAMBDA&& op) -> decltype(to) {
    std::transform(from.begin(), from.end(), std::back_inserter(to), op);
    return std::forward<decltype(to)>(to);
  }

  optvalptr genIfInst(ast::If& ast);

  static bool isPassByValue(types::Value const& type);

  int indent_counter = 0;
  int64_t varcounter = 0;
  std::string makeNewName();
  TypeEnv& typeenv;
  std::unordered_map<std::string, mir::valueptr> symbol_table;
  std::unordered_map<std::string, mir::valueptr> external_symbols;

  // static bool isExternalFun(std::string const& str) {
  //   return LLVMBuiltin::ftable.find(str) != LLVMBuiltin::ftable.end();
  // }
  mir::valueptr getOrGenExternalSymbol(std::string const& name, types::Value const& type);
  mir::valueptr getInternalSymbol(std::string const& name);
  optvalptr tryGetInternalSymbol(std::string const& name);
  mir::valueptr getFunctionSymbol(std::string const& name, types::Value const& type);
  // // unpack optional value ptr, and throw error if it does not exist.
  static mir::valueptr require(optvalptr const& v);

  std::function<std::shared_ptr<mir::Argument>(ast::DeclVar&)> make_arguments =
      [&](ast::DeclVar& lvar) {
        auto& name = lvar.value.value;
        auto& type = typeenv.find(name);
        auto res = std::make_shared<mir::Argument>(mir::Argument{name, type});
        symbol_table.emplace(name, std::make_shared<mir::Value>(res));
        return res;
      };
};

}  // namespace mimium