#include "interpreter_visitor.hpp"

namespace mimium {
double InterpreterVisitor::get_as_double(mValue v) {
  return std::visit(
      overloaded{[](double d) { return d; },
                 [this](std::string s) {
                   return get_as_double(this->findVariable(s));
                 },  // recursive!!
                 [](auto a) {
                   throw std::runtime_error("refered variable is not number");
                   return 0.;
                 }},
      v);
}
void InterpreterVisitor::init() {
  rootenv = std::make_shared<Environment>("root", nullptr);

  currentenv = rootenv;  // share
  currentenv->setVariable("dacL", 0);
  currentenv->setVariable("dacR", 0);
}
void InterpreterVisitor::clear() {
  rootenv.reset();
  currentenv.reset();
  clearDriver();
  init();
}

void InterpreterVisitor::start() {
  sch->start();
  running_status = true;
}

void InterpreterVisitor::stop() {
  sch->stop();
  running_status = false;
}

void InterpreterVisitor::loadSource(const std::string src) {
  driver.parsestring(src);
  loadAst(driver.getMainAst());
}
void InterpreterVisitor::loadSourceFile(const std::string filename) {
  driver.parsefile(filename);
  loadAst(driver.getMainAst());
}

void InterpreterVisitor::loadAst(AST_Ptr _ast) {
  auto ast = std::dynamic_pointer_cast<ListAST>(_ast);
  ast->accept(*this);
}

void InterpreterVisitor::visit(ListAST& ast) {
  for (auto& line : ast.getlist()) {
    line->accept(*this);
  }
}
void InterpreterVisitor::visit(NumberAST& ast) { vstack.push(ast.getVal()); };
void InterpreterVisitor::visit(SymbolAST& ast) { vstack.push(ast.getVal()); };
void InterpreterVisitor::visit(OpAST& ast) {
  ast.lhs->accept(*this);
  double lv = get_as_double(vstack.top());
  vstack.pop();
  ast.rhs->accept(*this);
  double rv = get_as_double(vstack.top());
  vstack.pop();
  double res;
  switch (ast.getOpId()) {
    case ADD:
      res = lv + rv;
      break;
    case SUB:
      res = lv - rv;
      break;
    case MUL:
      res = lv * rv;
      break;
    case DIV:
      res = lv / rv;
      break;
    case EXP:
      res = std::pow(lv, rv);
      break;
    case MOD:
      res = std::fmod(lv, rv);
      break;
    case AND:
    case BITAND:
      res = (double)((bool)lv & (bool)rv);
      break;
    case OR:
    case BITOR:
      res = (double)((bool)lv | (bool)rv);
      break;
    case LT:
      res = (double)lv < rv;
      break;
    case GT:
      res = (double)lv > rv;
      break;
    case LE:
      res = (double)lv <= rv;
      break;
    case GE:
      res = (double)lv >= rv;
      break;
    case LSHIFT:
      res = (double)((int)lv << (int)rv);
      break;
    case RSHIFT:
      res = (double)((int)lv >> (int)rv);
      break;
    default:
      throw std::runtime_error("invalid binary operator");
      res = 0.0;
  }
  vstack.push(res);
}
void InterpreterVisitor::visit(AssignAST& ast) {
  std::string varname = ast.getName()->getVal();  // assuming name as symbolast
  auto body = ast.getBody();

  if (body) {
    body->accept(*this);
    currentenv->setVariable(varname, vstack.top());
    Logger::debug_log(
        "Variable " + varname + " : " + InterpreterVisitor::to_string(vstack.top()),
        Logger::DEBUG);
    vstack.pop();  

  } else {
    throw std::runtime_error("expression not resolved");
  }
}

void InterpreterVisitor::visit(ArgumentsAST& ast) {
  // args = ast;//this visitor is not needed?
}
void InterpreterVisitor::visit(ArrayAST& ast) {
  std::vector<double> v;
  for (auto& elem : ast.getElements()) {
    elem->accept(*this);
    v.push_back(get_as_double(vstack.top()));
    vstack.pop();
  }
  vstack.push(std::move(v));
};
void InterpreterVisitor::visit(ArrayAccessAST& ast) {
  auto array =
      findVariable(ast.getName()->getVal());
  ast.getIndex()->accept(*this);
  auto index = (int)std::get<double>(vstack.top());
  vstack.pop();
  auto res=  std::visit(
      overloaded{[&index](std::vector<double> a) -> double { return a[index]; },
                 [](auto e) -> double {
                   throw std::runtime_error(
                       "accessed variable is not an array");
                   return 0;
                 }},
      array);
  vstack.push(res);
};
overloaded fcall_visitor{
    [](auto v) -> mClosure_ptr {
      throw std::runtime_error("reffered variable is not a function");
      return nullptr;
    },
    [](std::shared_ptr<Closure> v) -> mClosure_ptr { return v; }};
void InterpreterVisitor::visit(FcallAST& ast) {
  std::string name = ast.getFname()->getVal();
  auto args = ast.getArgs();
  if (Builtin::isBuiltin(name)) {
    auto fn = Builtin::builtin_fntable.at(name);
    vstack.push( fn(args, this) );
  } else {
    auto argsv = args->getElements();
    mClosure_ptr closure = std::visit(fcall_visitor, findVariable(name));
    auto lambda = closure->fun;
    auto originalenv = currentenv;
    auto lambdaargs = lambda.getArgs()->getElements();
    auto tmpenv =
        closure->env->createNewChild("arg" + name);  // create arguments
    int argscond = lambdaargs.size() - argsv.size();
    if (argscond < 0) {
      throw std::runtime_error("too many arguments");
    } else {
      int count = 0;
      for (auto& larg : lambdaargs) {
        auto key = std::dynamic_pointer_cast<SymbolAST>(larg)->getVal();
        argsv[count]->accept(*this);
        tmpenv->setVariableRaw(key,vstack.top());
        vstack.pop();
        count++;
      }
      if (argscond == 0) {
        currentenv = tmpenv;  // switch env
        lambda.getBody()->accept(*this);
        currentenv->getParent()->deleteLastChild();
        currentenv = originalenv;
      } else {
        throw std::runtime_error(
            "too few arguments");  // ideally we want to return new function
                                   // (partial application)
      }
    }
  }
};
void InterpreterVisitor::visit(LambdaAST& ast) {
  vstack.push( std::make_shared<Closure>(currentenv, ast) );
};
void InterpreterVisitor::visit(IfAST& ast) {
  ast.getCond()->accept(*this);
  mValue cond = vstack.top();
  vstack.pop();
  auto cond_d = get_as_double(cond);
  if (cond_d > 0) {
    ast.getThen()->accept(*this);
  } else {
    ast.getElse()->accept(*this);
  }
};
void InterpreterVisitor::visit(ReturnAST& ast) {
  ast.getExpr()->accept(*this);
};
void InterpreterVisitor::doForVisitor(mValue v, std::string iname,
                                      AST_Ptr expression) {
  std::visit(
      overloaded{
          [&](std::vector<double> v) {
            auto it = v.begin();
            while (it != v.end()) {
              currentenv->setVariable(iname, *it);
              expression->accept(*this);
              it++;
            }
          },
          [&](double v) {
            currentenv->setVariable(iname, v);
            expression->accept(*this);
          },
          [&](std::string s) { doForVisitor(findVariable(s),iname,expression); },
          [](auto v) { throw std::runtime_error("iterator is invalid"); }},
      v);
}

void InterpreterVisitor::visit(ForAST& ast) {
  std::string loopname = "for" + std::to_string(rand());  // temporary,,
  currentenv = currentenv->createNewChild(loopname);
  ast.getVar()->accept(*this);
  auto varname = std::get<std::string>(vstack.top());
  vstack.pop();
  auto expression = ast.getExpression();
  ast.getIterator()->accept(*this);
  mValue iterator = vstack.top();
  vstack.pop();
  doForVisitor(iterator,varname,expression);
  currentenv = currentenv->getParent();
};
void InterpreterVisitor::visit(DeclarationAST& ast) { 
  std::string name = ast.getFname()->getVal();
  auto argsast = ast.getArgs();
  auto args = argsast->getElements();
  if (name == "include") {
    assertArgumentsLength(args, 1);
    if (args[0]->getid() == SYMBOL) {//this is not smart
      args[0]->accept(*this);
      std::string filename = std::get<std::string>(vstack.top());
      vstack.pop();
      auto temporary_driver =
          std::make_unique<mmmpsr::MimiumDriver>(current_working_directory);
      temporary_driver->parsefile(filename);
      loadAst(temporary_driver->getMainAst());
    } else {
      throw std::runtime_error("given argument is not a string");
    }
  } else {
    throw std::runtime_error("specified declaration is not defined: " + name);
  }

 };
void InterpreterVisitor::visit(TimeAST& ast) {
  ast.getTime()->accept(*this);
  sch->addTask(get_as_double(vstack.top()), ast.getExpr());
  vstack.pop();
};

std::string InterpreterVisitor::to_string(mValue v) {
  return std::visit(overloaded{[](double v) { return std::to_string(v); },
                               [](std::vector<double> vec) {
                                 std::stringstream ss;
                                 ss << "[";
                                 int count = vec.size();
                                 for (auto& elem : vec) {
                                   ss << elem;
                                   count--;
                                   if (count > 0) ss << ",";
                                 }
                                 ss << "]";
                                 return ss.str();
                               },
                               [](std::string s) { return s; },
                               [](auto v) { return v->toString(); }},
                    v);
};

bool InterpreterVisitor::assertArgumentsLength(std::vector<AST_Ptr>& args,
                                               int length) {
  int size = args.size();
  if (size == length) {
    return true;
  } else {
    throw std::runtime_error(
        "Argument length is invalid. Expected: " + std::to_string(length) +
        " given: " + std::to_string(size));
    return false;
  };
}
}  // namespace mimium