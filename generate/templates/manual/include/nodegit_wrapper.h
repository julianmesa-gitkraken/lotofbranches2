#ifndef NODEGIT_WRAPPER_H
#define NODEGIT_WRAPPER_H

#include <algorithm>
#include <unordered_map>

#include "tracker_wrap.h"
#include "cleanup_handle.h"

// the Traits template parameter supplies:
//  typename cppClass - the C++ type of the NodeGit wrapper (e.g. GitRepository)
//  typename cType - the C type of the libgit2 object being wrapped (e.g. git_repository)
//
//  static const bool isDuplicable
//  static void duplicate(cType **dest, cType *src) - duplicates src using dupFunction or cpyFunction
//
//  static const bool isFreeable
//  static void free(cType *raw) - frees the object using freeFunctionName
//
// nodegit::TrackerWrap allows for cheap tracking of new objects, avoiding searchs
// in a container to remove the tracking of a specific object.

namespace nodegit {
  class Context;
}

template<typename Traits>
class NodeGitWrapper : public nodegit::TrackerWrap {
public:
  // replicate Traits typedefs for ease of use
  typedef typename Traits::cType cType;
  typedef typename Traits::cppClass cppClass;

  // whether raw should be freed on destruction
  // TODO: this should be protected but we have a few use cases that change this to
  // false from the outside.  I suspect it gets turned to false to avoid
  // double-free problems in cases like when we pass cred objects to libgit2
  // and it frees them.  We should probably be NULLing raw in that case
  // (and through a method) instead of changing selfFreeing, but that's
  // a separate issue.
  bool selfFreeing;

  nodegit::Context *nodegitContext = nullptr;

protected:
  cType *raw;
  std::vector<std::shared_ptr<nodegit::CleanupHandle>> childCleanupVector;

  // owner of the object, in the memory management sense. only populated
  // when using ownedByThis, and the type doesn't have a dupFunction
  // CopyablePersistentTraits are used to get the reset-on-destruct behavior.
  Nan::Persistent<v8::Object, Nan::CopyablePersistentTraits<v8::Object> > owner;

  // diagnostic count of self-freeing object instances
  thread_local static int SelfFreeingInstanceCount;
  // diagnostic count of constructed non-self-freeing object instances
  thread_local static int NonSelfFreeingConstructedCount;

  static void InitializeTemplate(v8::Local<v8::FunctionTemplate> &tpl);

  NodeGitWrapper(cType *raw, bool selfFreeing, v8::Local<v8::Object> owner);
  NodeGitWrapper(const char *error); // calls ThrowError
  NodeGitWrapper(const NodeGitWrapper &) = delete;
  NodeGitWrapper(NodeGitWrapper &&) = delete;
  NodeGitWrapper &operator=(const NodeGitWrapper &) = delete;
  NodeGitWrapper &operator=(NodeGitWrapper &&) = delete;
  ~NodeGitWrapper();

  static NAN_METHOD(JSNewFunction);

  static NAN_METHOD(GetSelfFreeingInstanceCount);
  static NAN_METHOD(GetNonSelfFreeingConstructedCount);

  void SetNativeOwners(v8::Local<v8::Object> owners);

public:
  static v8::Local<v8::Value> New(const cType *raw, bool selfFreeing, v8::Local<v8::Object> owner = v8::Local<v8::Object>());

  void SaveCleanupHandle(std::shared_ptr<nodegit::CleanupHandle> cleanupHandle);

  void Reference();
  void Unreference();

  void AddReferenceCallbacks(size_t, std::function<void()>, std::function<void()>);

  cType *GetValue();
  void ClearValue();

private:
  std::unordered_map<size_t, std::function<void()>> referenceCallbacks;
  std::unordered_map<size_t, std::function<void()>> unreferenceCallbacks;
};

#endif
