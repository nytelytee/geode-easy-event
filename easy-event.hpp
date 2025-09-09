/*
 * This is free and unencumbered software released into the public domain.
 * 
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any
 * means.
 * 
 * In jurisdictions that recognize copyright laws, the author or authors
 * of this software dedicate any and all copyright interest in the
 * software to the public domain. We make this dedication for the benefit
 * of the public at large and to the detriment of our heirs and
 * successors. We intend this dedication to be an overt act of
 * relinquishment in perpetuity of all present and future rights to this
 * software under copyright law.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 * 
 * For more information, please refer to <https://unlicense.org>
 */

#pragma once

#include <Geode/loader/Dispatch.hpp>

namespace nytelyte::easy_event {
  
  namespace internal {

    template<size_t N>
    struct StringLiteral {
      constexpr StringLiteral(const char (&str)[N]) { std::copy_n(str, N, value); }
      char value[N];
    };

    template<typename Function, typename... EventParameters>
    concept FunctionHasValidArguments = std::invocable<Function, EventParameters...>;
    
    template<typename Function, typename ReturnType, typename... EventParameters>
    concept FunctionHasValidReturnType = (
      (std::is_void_v<ReturnType> && std::is_void_v<std::invoke_result_t<Function, EventParameters...>>) ||
      std::assignable_from<ReturnType&, std::invoke_result_t<Function, EventParameters...>>
    );

    template <typename Function, typename ReturnType, typename... EventParameters>
    concept IsValidFunction = (
      FunctionHasValidArguments<Function, EventParameters...> &&
      FunctionHasValidReturnType<Function, ReturnType, EventParameters...>
    );
    
    template <typename Class, typename ReturnType, typename... EventParameters>
    using MemberFunction = typename geode::to_member<Class, ReturnType(EventParameters...)>::value;

    template<typename Class, typename ReturnType, typename... EventParameters>
    concept MemberFunctionHasValidReturnType = (
      (std::is_void_v<ReturnType> && std::is_void_v<std::invoke_result_t<MemberFunction<Class, ReturnType, EventParameters...>, Class*, EventParameters...>>) ||
      std::assignable_from<ReturnType&, std::invoke_result_t<MemberFunction<Class, ReturnType, EventParameters...>, Class*, EventParameters...>>
    );
    
    template <typename Class, typename ReturnType, typename... EventParameters>
    concept IsValidMemberFunction = MemberFunctionHasValidReturnType<Class, ReturnType, EventParameters...>;

    template<typename EventID, typename EventParameters, typename EventReturn>
    struct EventImpl;

    template<typename EventID, typename... EventParameters, typename EventReturn>
    struct EventImpl<EventID, std::tuple<EventParameters...>, EventReturn> {
    private:
      // needed for the macros, since you can't pass things which have commas in them lol
      using ReturnValuePair = std::pair<geode::ListenerResult, EventReturn>;
    public:

      // actual internal dispatch event values, in case you want to use them
      using Filter = std::conditional_t<
          std::is_void_v<EventReturn>,
          geode::DispatchFilter<EventParameters...>,
          geode::DispatchFilter<EventReturn*, EventParameters...>
      >;
      using Event = Filter::Event;
      using Listener = geode::EventListener<Filter>;

      // the parameters the EasyEvent is using right now, in case you need them
      static constexpr const char* ID = std::is_void_v<EventID> ? nullptr : EventID::value;
      using Returns = EventReturn;
      using Takes = std::tuple<EventParameters...>;

      // normal post
      inline static geode::ListenerResult post(const char* id, EventParameters... args) {
        if constexpr (std::is_void_v<EventReturn>) return Event(id, args...).post();
        else return Event(id, nullptr, args...).post();
      }

      // receiving post
      inline static EventReturn receive(const char* id, EventParameters... args) requires (!std::is_void_v<EventReturn>)  {
        EventReturn into;
        Event(id, &into, args...).post();
        return into;
      }

      // receiving both post
      inline static std::pair<geode::ListenerResult, EventReturn> receiveBoth(const char* id, EventParameters... args) requires (!std::is_void_v<EventReturn>)  {
        EventReturn into;
        geode::ListenerResult result = Event(id, &into, args...).post();
        return std::pair{result, into};
      }
      
      // if the easy event has an embedded id, these post/receive/receiveBoth overloads are defined, allowing you to post the event without an id
      inline static geode::ListenerResult post(EventParameters... args) requires (!std::is_void_v<EventID>) { return post(EventImpl::ID, args...); }
      inline static EventReturn receive(EventParameters... args) requires (!std::is_void_v<EventID>)  { return receive(EventImpl::ID, args...); }
      inline static std::pair<geode::ListenerResult, EventReturn> receiveBoth(EventParameters... args) requires (!std::is_void_v<EventID>)  { return receiveBoth(EventImpl::ID, args...); }

      // note: all listeners have member function versions as well, to mirror geode's event listeners, which also provide this behavior
      
      // preset result listening
      template <geode::ListenerResult result, typename Function> requires IsValidFunction<Function, void, EventParameters...>
      inline static Listener listen(const char* id, Function&& function) {
        if constexpr (std::is_void_v<EventReturn>)
          return Listener(
            [func = std::forward<Function>(function)](EventParameters... args) {
              func(args...);
              return result;
            },
            Filter(id)
          );
        else
          // the underlying event will receive the return value pointer, but will do nothing with it
          return Listener(
            [func = std::forward<Function>(function)](EventReturn*, EventParameters... args) {
              func(args...);
              return result;
            },
            Filter(id)
          );
      }

      // unpreset result listening
      template <typename Function> requires IsValidFunction<Function, geode::ListenerResult, EventParameters...>
      inline static Listener listen(const char* id, Function&& function) {
        if constexpr (std::is_void_v<EventReturn>)
          return Listener(std::forward<Function>(function), Filter(id));
        else
          // the underlying event will receive the return value pointer, but will do nothing with it
          return Listener(
            [func = std::forward<Function>(function)](EventReturn*, EventParameters... args) {
              return func(args...);
            },
            Filter(id)
          );
      }

      // preset result sending
      template <geode::ListenerResult result, typename Function> requires (!std::is_void_v<EventReturn> && IsValidFunction<Function, EventReturn, EventParameters...>)
      inline static Listener send(const char* id, Function&& function) {
        return Listener(
          [func = std::forward<Function>(function)](EventReturn* into, EventParameters... args) {
            if (into) *into = func(args...);
            else func(args...);
            return result;
          },
          Filter(id)
        );
      }

      // unpreset result sending
      template <typename Function> requires (!std::is_void_v<EventReturn> && IsValidFunction<Function, std::pair<geode::ListenerResult, EventReturn>, EventParameters...>)
      inline static Listener send(const char* id, Function&& function) {
        return Listener(
          [func = std::forward<Function>(function)](EventReturn* into, EventParameters... args) {
            geode::ListenerResult result;
            if (into) std::tie(result, *into) = func(args...);
            else std::tie(result, std::ignore) = func(args...);
            return result;
          },
          Filter(id)
        );
      }
      
      // MEMBER FUNCTIONS

      // preset result listening
      template<geode::ListenerResult result, typename Class> requires IsValidMemberFunction<Class, void, EventParameters...>
      inline static Listener listenOn(const char* id, Class* instance, MemberFunction<Class, void, EventParameters...> function) {
        if constexpr (std::is_void_v<EventReturn>)
          return Listener(
            [instance, function](EventParameters... args) {
              (instance->*function)(args...);
              return result;
            },
            Filter(id)
          );
        else
          // the underlying event will receive the return value pointer, but will do nothing with it
          return Listener(
            [instance, function](EventReturn*, EventParameters... args) {
              (instance->*function)(args...);
              return result;
            },
            Filter(id)
          );
      }
      
      // unpreset result listening
      template<typename Class> requires IsValidMemberFunction<Class, geode::ListenerResult, EventParameters...>
      inline static Listener listenOn(const char* id, Class* instance, MemberFunction<Class, geode::ListenerResult, EventParameters...> function) {
        if constexpr (std::is_void_v<EventReturn>)
          return Listener(instance, function, Filter(id));
        else
          // the underlying event will receive the return value pointer, but will do nothing with it
          return Listener(
            [instance, function](EventReturn*, EventParameters... args) {
              return (instance->*function)(args...);
            },
            Filter(id)
          );
      }

      // preset result sending
      template<geode::ListenerResult result, typename Class> requires (!std::is_void_v<EventReturn> && IsValidMemberFunction<Class, EventReturn, EventParameters...>)
      inline static Listener sendOn(const char* id, Class* instance, MemberFunction<Class, EventReturn, EventParameters...> function) {
        return Listener(
          [instance, function](EventReturn* into, EventParameters... args) {
            if (into) *into = (instance->*function)(args...);
            else (instance->*function)(args...);
            return result;
          },
          Filter(id)
        );
      }
      
      // unpreset result sending
      template<typename Class> requires (!std::is_void_v<EventReturn> && IsValidMemberFunction<Class, std::pair<geode::ListenerResult, EventReturn>, EventParameters...>)
      inline static Listener sendOn(const char* id, Class* instance, MemberFunction<Class, std::pair<geode::ListenerResult, EventReturn>, EventParameters...> function) {
        return Listener(
          [instance, function](EventReturn* into, EventParameters... args) {
            geode::ListenerResult result;
            if (into) std::tie(result, *into) = (instance->*function)(args...);
            else std::tie(result, std::ignore) = (instance->*function)(args...);
            return result;
          },
          Filter(id)
        );
      }

      // "Non-base" listener functions; they implement behavior such as global listeners, embedded id insertion, or both at the same time
      // they also all have preset result and unpreset result versions, and member function versions of those
      // I dislike how unreadable this is, but there are a lot of functions, and writing them out manually would be even less readable and error-prone
      
      // global listener which takes an event id, a base global listener
      #define CREATE_MAIN_GLOBAL_LISTENER(Name, name, ReturnTypeTemplated, ReturnTypeUntemplated) \
        template <geode::ListenerResult result, typename Function> requires IsValidFunction<Function, ReturnTypeTemplated, EventParameters...>\
        inline static Listener* global##Name(const char* id, Function&& function)\
        { return new Listener(name<result, Function>(id, std::forward<Function>(function))); }\
        \
        template <typename Function> requires IsValidFunction<Function, ReturnTypeUntemplated, EventParameters...>\
        inline static Listener* global##Name(const char* id, Function&& function)\
        { return new Listener(name<Function>(id, std::forward<Function>(function))); }

      // regular listener with an embedded id
      #define CREATE_LISTENER_WITH_EVENT_ID(name, ReturnTypeTemplated, ReturnTypeUntemplated) \
        template <geode::ListenerResult result, typename Function> requires IsValidFunction<Function, ReturnTypeTemplated, EventParameters...>\
        inline static Listener name(Function&& function)\
        { return name<result, Function>(EventImpl::ID, std::forward<Function>(function)); }\
        \
        template <typename Function> requires IsValidFunction<Function, ReturnTypeUntemplated, EventParameters...>\
        inline static Listener name(Function&& function)\
        { return name<Function>(EventImpl::ID, std::forward<Function>(function)); }

      // global listener with an embedded id
      #define CREATE_GLOBAL_LISTENER_WITH_EVENT_ID(Name, ReturnTypeTemplated, ReturnTypeUntemplated) \
        template <geode::ListenerResult result, typename Function> requires IsValidFunction<Function, ReturnTypeTemplated, EventParameters...>\
        inline static Listener* global##Name(Function&& function)\
        { return global##Name<result, Function>(EventImpl::ID, std::forward<Function>(function)); }\
        \
        template <typename Function> requires IsValidFunction<Function, ReturnTypeUntemplated, EventParameters...>\
        inline static Listener* global##Name(Function&& function)\
        { return global##Name<Function>(EventImpl::ID, std::forward<Function>(function)); }

      // global member function listener which takes an event id, a base global member function listener
      #define CREATE_MAIN_GLOBAL_MEMBER_LISTENER(Name, name, ReturnTypeTemplated, ReturnTypeUntemplated) \
        template <geode::ListenerResult result, typename Class> requires IsValidMemberFunction<Class, ReturnTypeTemplated, EventParameters...>\
        inline static Listener* global##Name(const char* id, Class* instance, MemberFunction<Class, ReturnTypeTemplated, EventParameters...> function)\
        { return new Listener(name<result, Class>(id, instance, function)); }\
        \
        template <typename Class> requires IsValidMemberFunction<Class, ReturnTypeUntemplated, EventParameters...>\
        inline static Listener* global##Name(const char* id, Class* instance, MemberFunction<Class, ReturnTypeUntemplated, EventParameters...> function)\
        { return new Listener(name<Class>(id, instance, function)); }

      // regular member function listener with an embedded id
      #define CREATE_MEMBER_LISTENER_WITH_EVENT_ID(name, ReturnTypeTemplated, ReturnTypeUntemplated) \
        template <geode::ListenerResult result, typename Class> requires IsValidMemberFunction<Class, ReturnTypeTemplated, EventParameters...>\
        inline static Listener name(Class* instance, MemberFunction<Class, ReturnTypeTemplated, EventParameters...> function)\
        { return name<result, Class>(EventImpl::ID, instance, function); }\
        \
        template <typename Class> requires IsValidMemberFunction<Class, ReturnTypeUntemplated, EventParameters...>\
        inline static Listener name(Class* instance, MemberFunction<Class, ReturnTypeUntemplated, EventParameters...> function)\
        { return name<Class>(EventImpl::ID, instance, function); }

      // global member function listener with an embedded id
      #define CREATE_GLOBAL_MEMBER_LISTENER_WITH_EVENT_ID(Name, ReturnTypeTemplated, ReturnTypeUntemplated) \
        template <geode::ListenerResult result, typename Class> requires IsValidMemberFunction<Class, ReturnTypeTemplated, EventParameters...>\
        inline static Listener* global##Name(Class* instance, MemberFunction<Class, ReturnTypeTemplated, EventParameters...> function)\
        { return global##Name<result, Class>(EventImpl::ID, instance, function); }\
        \
        template <typename Class> requires IsValidMemberFunction<Class, ReturnTypeUntemplated, EventParameters...>\
        inline static Listener* global##Name(Class* instance, MemberFunction<Class, ReturnTypeUntemplated, EventParameters...> function)\
        { return global##Name<Class>(EventImpl::ID, instance, function); }
      
      // global base listeners
      CREATE_MAIN_GLOBAL_LISTENER(Listen, listen, void, geode::ListenerResult);
      CREATE_MAIN_GLOBAL_LISTENER(Send, send, EventReturn, ReturnValuePair);
      
      // non-sending listeners with an embedded id
      CREATE_LISTENER_WITH_EVENT_ID(listen, void, geode::ListenerResult);
      CREATE_GLOBAL_LISTENER_WITH_EVENT_ID(Listen, void, geode::ListenerResult);

      // sending listeners with an embedded id
      CREATE_LISTENER_WITH_EVENT_ID(send, EventReturn, ReturnValuePair);
      CREATE_GLOBAL_LISTENER_WITH_EVENT_ID(Send, EventReturn, ReturnValuePair);
       
      // global base member function listeners
      CREATE_MAIN_GLOBAL_MEMBER_LISTENER(ListenOn, listenOn, void, geode::ListenerResult);
      CREATE_MAIN_GLOBAL_MEMBER_LISTENER(SendOn, sendOn, EventReturn, ReturnValuePair);

      // non-sending member function listeners with an embedded id
      CREATE_MEMBER_LISTENER_WITH_EVENT_ID(listenOn, void, geode::ListenerResult);
      CREATE_GLOBAL_MEMBER_LISTENER_WITH_EVENT_ID(ListenOn, void, geode::ListenerResult);

      // sending member function listeners with an embedded id
      CREATE_MEMBER_LISTENER_WITH_EVENT_ID(sendOn, EventReturn, ReturnValuePair);
      CREATE_GLOBAL_MEMBER_LISTENER_WITH_EVENT_ID(SendOn, EventReturn, ReturnValuePair);

      #undef CREATE_MAIN_GLOBAL_LISTENER
      #undef CREATE_LISTENER_WITH_EVENT_ID
      #undef CREATE_GLOBAL_LISTENER_WITH_EVENT_ID
      #undef CREATE_MAIN_GLOBAL_MEMBER_LISTENER
      #undef CREATE_MEMBER_LISTENER_WITH_EVENT_ID
      #undef CREATE_GLOBAL_MEMBER_LISTENER_WITH_EVENT_ID
      
    };

    template<typename T>
    struct is_tuple : std::false_type {};

    template<typename... Args>  
    struct is_tuple<std::tuple<Args...>> : std::true_type {};

    template<typename EventID = void, typename EventParameters = std::tuple<>, typename EventReturn = void>
    struct EventBuilder : public EventImpl<EventID, EventParameters, EventReturn> {
    private:
      template<StringLiteral... Id>
      static constexpr auto makeIdType() {
        if constexpr (sizeof...(Id) == 0)
          return static_cast<EventBuilder<void, EventParameters, EventReturn>*>(nullptr);
        else if constexpr (sizeof...(Id) == 1)
          return static_cast<EventBuilder<std::integral_constant<const char*, Id.value>..., EventParameters, EventReturn>*>(nullptr);
        else
          static_assert(sizeof...(Id) <= 1, "id<> accepts at most one string literal");
      }

      template<typename T>
      struct takes_tuple_helper {
          static_assert(is_tuple<T>::value, 
                        "takes_tuple requires a std::tuple<Args...> type");
          using type = EventBuilder<EventID, T, EventReturn>;
      };

    public:
      EventBuilder() = delete;
      EventBuilder(const EventBuilder&) = delete;
      EventBuilder(EventBuilder&&) = delete;
      EventBuilder& operator=(const EventBuilder&) = delete;
      EventBuilder& operator=(EventBuilder&&) = delete;
      ~EventBuilder() = delete;
      
      // allows you to set the dispatch event id as part of the type itself, so you only have to write it once; now you can't misspell it.
      // you can remove the id via ::id<>, in case you want to do that for some reason, implemented in makeIdType
      template<StringLiteral... Id>
      using id = std::remove_pointer_t<decltype(makeIdType<Id...>())>;
      
      // you pass in the types of the parameters that the event takes, in order, the same way you would on a geode::DispatchEvent
      template<typename... Parameters>
      using takes = EventBuilder<EventID, std::tuple<Parameters...>, EventReturn>;
      
      // added in case you want to, say, copy a different EasyEvent's Takes values, you could do EasyEvent::takes_tuple<MyEvent::Takes>.
      // i could just pass ParameterTuple directly to EventBuilder, but the error messages suck, so we're doing it like this
      // since i can actually add a static assert into takes_tuple_helper
      template<typename ParameterTuple = std::tuple<>>
      using takes_tuple = typename takes_tuple_helper<ParameterTuple>::type;
      
      // an EasyEvent may have a return value, passed via pointer, the underlying geode event receives that pointer as the first argument
      // you can remove the return value (which is the same as setting it to void, which you can also do), by passing ::returns<> or ::returns<void>
      template<typename Return = void>
      using returns = EventBuilder<EventID, EventParameters, Return>;
    };

  }
  using EasyEvent = internal::EventBuilder<>;
  
}
