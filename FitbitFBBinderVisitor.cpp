//
// FitbitFBBinderVisitor.cpp
// Created by Jonathon Mah on 2014-01-25.
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//

#include "FitbitFBBinderVisitor.h"

using namespace clang;


FBBinderVisitor::FBBinderVisitor(KeyPathValidationConsumer *Consumer, const CompilerInstance &Compiler)
    : Consumer(Consumer)
    , Compiler(Compiler)
{
  ASTContext &Ctx = Compiler.getASTContext();
  IdentifierTable &IDs = Ctx.Idents;
  IdentifierInfo *BindIIs[3] = {&IDs.get("bindToModel"), &IDs.get("keyPath"), &IDs.get("change")};
  BindSelector = Ctx.Selectors.getSelector(3, BindIIs);

  IdentifierInfo *BindMultipleIIs[3] = {&IDs.get("bindToModels"), &IDs.get("keyPaths"), &IDs.get("change")};
  BindMultipleSelector = Ctx.Selectors.getSelector(3, BindMultipleIIs);
  BindMultipleCountMismatchDiagID = Compiler.getDiagnostics().getCustomDiagID(DiagnosticsEngine::Error, "model and key path arrays must have same number of elements");
}

bool FBBinderVisitor::VisitObjCMessageExpr(ObjCMessageExpr *E) {
  if (E->getNumArgs() != 3 || !E->isInstanceMessage())
    return true;

  if (E->getSelector() == BindSelector)
    Consumer->emitDiagnosticsForReceiverAndKeyPath(E->getArg(0), E->getArg(1));

  if (E->getSelector() == BindMultipleSelector) {
    ObjCArrayLiteral *ModelsLiteral = dyn_cast<ObjCArrayLiteral>(E->getArg(0)->IgnoreImplicit());
    ObjCArrayLiteral *KeyPathsLiterals = dyn_cast<ObjCArrayLiteral>(E->getArg(1)->IgnoreImplicit());

    if (ModelsLiteral && KeyPathsLiterals) {
      if (ModelsLiteral->getNumElements() == KeyPathsLiterals->getNumElements()) {
        for (unsigned ModelIdx = 0, ModelCount = ModelsLiteral->getNumElements();
        ModelIdx < ModelCount; ++ModelIdx) {
          Expr *ModelExpr = ModelsLiteral->getElement(ModelIdx);
          Expr *KeyPathsExpr = KeyPathsLiterals->getElement(ModelIdx);

          if (ObjCArrayLiteral *KeyPathsLiteral = dyn_cast<ObjCArrayLiteral>(KeyPathsExpr->IgnoreImplicit())) {
            for (unsigned KeyPathIdx = 0, KeyPathCount = KeyPathsLiteral->getNumElements();
            KeyPathIdx < KeyPathCount; ++KeyPathIdx) {
              Expr *KeyPathExpr = KeyPathsLiteral->getElement(KeyPathIdx);
              Consumer->emitDiagnosticsForReceiverAndKeyPath(ModelExpr, KeyPathExpr);
            }
          }
        }

      } else {
        Compiler.getDiagnostics().Report(ModelsLiteral->getLocStart(), BindMultipleCountMismatchDiagID)
          << ModelsLiteral << KeyPathsLiterals;
      }
    }
  }

  return true;
}
