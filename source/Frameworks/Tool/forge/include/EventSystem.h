#pragma once

#include <cstdint>
#include <memory>
#include <functional>
#include <optional>
#include <unordered_map>
#include <acl/detail/type_name.hpp>
#include <acl/sparse_table.hpp>

namespace terra
{

class TerraMainApp;

template <typename Ev>
static inline constexpr bool ReplaceLatest = true;

template <typename Ev, bool B>
struct Collector;

template <typename Ev>
struct Collector<Ev, true>
{
  template <typename L>
  void dispatch(L& table, TerraMainApp& app)
  {
    if (saved.has_value())
    {
      table.for_each([this, &app](auto, auto& lambda) {
        lambda(app, saved.value());
      });
      saved.reset();
    }
  }

  void accept(Ev const& value)
  {
    saved.reset();
    saved = value;
  }

  std::optional<Ev> saved;
};

template <typename Ev>
struct Collector<Ev, false>
{
  template <typename L>
  void dispatch(L& table, TerraMainApp& app)
  {
    if (!saved.empty())
    {
      table.for_each([this, &app](auto, auto& lambda) {
        for(auto const& e : saved)
          lambda(app, e);
      });
      saved.clear();
    }
  }

  void accept(Ev const& value)
  {
    saved.emplace_back(value);
  }

  std::vector<Ev> saved;
};

class EventSystem
{

  template<typename T>
  struct ListenerTraits;

  template<typename Ev>
  struct ListenerTraits<std::function<void(TerraMainApp&, Ev const&)>>
  {
    using EventType = Ev;
  };

  template <typename T>
  struct ListenerTraits : public ListenerTraits<decltype(&T::operator())>
  {};

  template <typename ClassType, typename Arg>
  struct ListenerTraits<void (ClassType::*)(TerraMainApp&, Arg const&) const>
  // we specialize for pointers to member function
  {
    using EventType = Arg;
  };

  struct DispatcherBase
  {
    inline virtual ~DispatcherBase() {}
    virtual void dispatch(TerraMainApp&) = 0;
    virtual void remove(uint32_t) = 0;
  };

  template <typename Ev>
  struct Dispatcher : DispatcherBase
  {
    using Listener = std::function<void(TerraMainApp&, Ev const&)>;

    void accept(Ev const& ev)
    {
      collector.accept(ev);
    }

    void dispatch(TerraMainApp& app) final
    {
      collector.dispatch(listeners, app);
    }

    template <typename L>
    uint32_t add(L&& lambda)
    {
      auto id = listeners.emplace(std::forward<L>(lambda));
      return (uint32_t)id;
    }

    void remove(uint32_t id) final
    {
      listeners.remove(typename acl::sparse_table<Listener>::link(id));
    }

    Collector<Ev, ReplaceLatest<Ev>> collector;
    acl::sparse_table<Listener> listeners;
  };

  template <typename Ev>
  Dispatcher<Ev>& ensure()
  {
    auto h = acl::detail::type_hash<Ev>();
    auto it = dispatchers.find(h);

    if (it == dispatchers.end())
    {
      auto shared = std::make_shared<Dispatcher<Ev>>();
      dispatchers.emplace(h, std::static_pointer_cast<DispatcherBase>(shared));
      return *shared.get();
    }
    else
      return static_cast<Dispatcher<Ev>&>(*it->second.get());
  }


public:

  template <typename L>
  uint64_t listen(L&& lambda)
  {    
    using Ev = typename ListenerTraits<L>::EventType;
    uint64_t h = acl::detail::type_hash<Ev>();
    return (uint64_t)ensure<Ev>().add(std::forward<L>(lambda)) | h << 32;
  }

  void remove(uint64_t listener)
  {
    auto id = static_cast<uint32_t>(listener & 0xffffffff);
    auto h = static_cast<uint32_t>(listener >> 32);
    auto it = dispatchers.find(h);
    if (it != dispatchers.end())
      it->second->remove(id);
  }

  template <typename Ev>
  void enqueue(Ev const& event)
  {
    auto& d = ensure<Ev>();
    d.accept(event);
  }

  inline void dispatch(TerraMainApp& app)
  {
    for(auto& e : dispatchers)
      e.second->dispatch(app);
  }

private:
  std::unordered_map<std::uint32_t,  std::shared_ptr<DispatcherBase>> dispatchers;
};

}
