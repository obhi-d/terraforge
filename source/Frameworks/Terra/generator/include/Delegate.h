
#pragma once

#include <functional>

namespace terra
{

struct Delegate
{
  static constexpr uintptr_t tag_mask = 3;

  using Run = void (*)(void*, void*);

  template <auto Method, typename C>
  static Delegate From(C* const _ptr) noexcept
  {
    return Delegate(_ptr,
                    [](void* data, void*)
                    {
                      auto self = reinterpret_cast<C*>(data);
                      std::invoke(Method, self);
                    });
  }

  template <auto Method, typename C, typename P>
  static Delegate From(C* const _ptr, P* const _param) noexcept
  {
    return Delegate(_ptr,
                    [](void* data, void* param)
                    {
                      auto self  = reinterpret_cast<C*>(data);
                      auto param = reinterpret_cast<P*>(param);
                      std::invoke(Method, self, param);
                    });
  }

  static Delegate From(void* const _ptr, void* const _param, Run method) noexcept
  {
    return Delegate(_ptr, _param, method);
  }

  template <auto Method>
  static Delegate From() noexcept
  {
    return Delegate(nullptr,
                    [](void*, void*)
                    {
                      Method();
                    });
  }

  inline Delegate(void* selfp, Run method) : self(selfp), runner(method) {}
  inline Delegate(void* selfp, void* paramp, Run method) : self(selfp), param(paramp), runner(method) {}

  inline void operator()
  {
    runner(data, param);
  }

  Run   runner;
  void* self  = nullptr;
  void* param = nullptr;
};

} // namespace terra