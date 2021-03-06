//
// KeyPathValidationConsumer.h
// Created by Jonathon Mah on 2014-01-24.
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//

#ifndef CLANG_KPV_KEY_PATH_VALIDATION_CONSUMER_H
#define CLANG_KPV_KEY_PATH_VALIDATION_CONSUMER_H

#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/NSAPI.h"
#include "clang/AST/Attr.h"
#include "clang/Frontend/CompilerInstance.h"

using namespace clang;

class KeyPathValidationConsumer : public ASTConsumer {
public:
  KeyPathValidationConsumer(const CompilerInstance &Compiler)
    : ASTConsumer()
    , Compiler(Compiler)
    , Context(Compiler.getASTContext())
  {
    NSAPIObj.reset(new NSAPI(Context));
	DiagnosticsEngine::Level L = DiagnosticsEngine::Warning;
	if (Compiler.getDiagnostics().getWarningsAsErrors())
	  L = DiagnosticsEngine::Error;
    KeyDiagID = Compiler.getDiagnostics().getCustomDiagID(L, "key '%0' not found on type %1");
  }

  virtual void HandleTranslationUnit(ASTContext &Context);

  bool CheckKeyType(QualType &ObjTypeInOut, StringRef &Key, bool AllowPrivate);

  void emitDiagnosticsForReceiverAndKeyPath(const Expr *ModelExpr, const Expr *KeyPathExpr, bool AllowPrivate=false) {
    emitDiagnosticsForTypeAndMaybeReceiverAndKeyPath(ModelExpr->IgnoreImplicit()->getType(), ModelExpr, KeyPathExpr, AllowPrivate);
  }

  void emitDiagnosticsForTypeAndKey(QualType Type, const Expr *KeyExpr, bool AllowPrivate=false) {
    const ObjCStringLiteral *KeyPathLiteral = dyn_cast<ObjCStringLiteral>(KeyExpr);
    if (KeyPathLiteral)
      emitDiagnosticsForTypeAndMaybeReceiverAndKey(Type, SourceRange(), KeyPathLiteral->getString()->getString(), KeyExpr->getSourceRange(), 0, AllowPrivate);
  }

  void emitDiagnosticsForTypeAndKeyPath(QualType Type, const Expr *KeyPathExpr, bool AllowPrivate=false) {
    emitDiagnosticsForTypeAndMaybeReceiverAndKeyPath(Type, NULL, KeyPathExpr, AllowPrivate);
  }

  unsigned KeyDiagID;

private:
  const CompilerInstance &Compiler;
  ASTContext &Context;
  OwningPtr<NSAPI> NSAPIObj;

  QualType NSNumberPtrType;

  // Hard-coded set of KVC containers (can't add attributes in a category)
  ObjCInterfaceDecl *NSDictionaryInterface, *NSArrayInterface, *NSSetInterface, *NSOrderedSetInterface;

  void cacheNSTypes();
  bool isKVCContainer(QualType type);
  bool isKVCCollectionType(QualType type);

  void emitDiagnosticsForTypeAndMaybeReceiverAndKeyPath(QualType Type, const Expr *ModelExpr, const Expr *KeyPathExpr, bool AllowPrivate);
  bool emitDiagnosticsForTypeAndMaybeReceiverAndKey(QualType &ObjTypeInOut, SourceRange ModelRange, StringRef Key, SourceRange KeyRange, size_t Offset, bool AllowPrivate);
};

#endif
