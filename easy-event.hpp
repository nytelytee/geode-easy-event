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

    template <size_t N>
    struct StringLiteral {
      constexpr StringLiteral(const char (&str)[N]) { std::copy_n(str, N, value); }
      char value[N];
    };

    template <typename Function, typename... EventParameters>
    concept FunctionHasValidArguments = std::invocable<Function, EventParameters...>;

    template <typename Function, typename ReturnType, typename... EventParameters>
    concept FunctionHasValidReturnType = (
      (std::is_void_v<ReturnType> && std::is_void_v<std::invoke_result_t<Function, EventParameters...>>) ||
      std::assignable_from<ReturnType&, std::invoke_result_t<Function, EventParameters...>>
    );

    template <typename Function, typename ReturnType, typename... EventParameters>
    concept IsValidFunction = (
      FunctionHasValidArguments<Function, EventParameters...> &&
      FunctionHasValidReturnType<Function, ReturnType, EventParameters...>
    );

    template <typename EventID, typename EventParameters, typename EventReturn>
    struct EventImpl;

    template <typename EventID, typename... EventParameters, typename EventReturn>
    struct EventImpl<EventID, std::tuple<EventParameters...>, EventReturn> {
    private:
      using EventReturnPointer = std::conditional_t<std::is_void_v<EventReturn>, std::nullptr_t, EventReturn*>;
    public:
      // Actual internal dispatch event value
      using Event = std::conditional_t<
          std::is_void_v<EventReturn>,
          geode::Dispatch<EventParameters...>,
          geode::Dispatch<EventReturn*, EventParameters...>
      >;

      // The parameters the EasyEvent is using right now, in case you need them
      static constexpr const char* ID = []{ if constexpr (std::is_void_v<EventID>) return nullptr; else return EventID::value; }();
      using Returns = EventReturn;
      using Takes = std::tuple<EventParameters...>;

      /****************************/
      /* Base versions of senders */
      /****************************/

      // Normal send
      inline static bool sendWithID(const char* id, EventParameters... args) {
        if constexpr (std::is_void_v<EventReturn>) return Event(id).send(args...);
        else return Event(id).send(nullptr, args...);
      }
      
      // Raw send
      inline static bool rawSendWithID(const char* id, EventReturnPointer into, EventParameters... args) {
        if constexpr (std::is_void_v<EventReturn>) return Event(id).send(args...);
        else return Event(id).send(into, args...);
      }

      // Send and receive
      inline static EventReturn sendAndReceiveWithID(const char* id, EventParameters... args)
      requires (!std::is_void_v<EventReturn>)  {
        EventReturn into;
        Event(id).send(&into, args...);
        return into;
      }

      // Send and receive both
      inline static std::pair<bool, EventReturn> sendAndReceiveBothWithID(const char* id, EventParameters... args)
      requires (!std::is_void_v<EventReturn>) {
        EventReturn into;
        bool result = Event(id).send(&into, args...);
        return std::pair{result, into};
      }
      
      /*********************************/
      /* Preset ID versions of senders */
      /*********************************/

      inline static bool send(EventParameters... args)
      requires (!std::is_void_v<EventID>)
      { return sendWithID(EventImpl::ID, args...); }
      
      inline static bool rawSend(EventReturnPointer into, EventParameters... args)
      requires (!std::is_void_v<EventID>)
      { return rawSendWithID(EventImpl::ID, into, args...); }

      inline static EventReturn sendAndReceive(EventParameters... args)
      requires (!std::is_void_v<EventID> && !std::is_void_v<EventReturn>)
      { return sendAndReceiveWithID(EventImpl::ID, args...); }

      inline static std::pair<bool, EventReturn> sendAndReceiveBoth(EventParameters... args)
      requires (!std::is_void_v<EventID> && !std::is_void_v<EventReturn>)
      { return sendAndReceiveBothWithID(EventImpl::ID, args...); }

      /******************************/
      /* Base versions of listeners */
      /******************************/

      // Preset result listening
      template <bool stop, typename Function>
      requires IsValidFunction<Function, void, EventParameters...>
      inline static geode::ListenerHandle listenWithID(const char* id, Function&& function, int priority = 0) {
        if constexpr (std::is_void_v<EventReturn>)
          return Event(id).listen(
            [func = std::forward<Function>(function)](EventParameters... args) { func(args...); return stop; },
            priority
          );
        else
          // the underlying event will receive the return value pointer, but will do nothing with it
          return Event(id).listen(
            [func = std::forward<Function>(function)](EventReturn*, EventParameters... args) { func(args...); return stop; },
            priority
          );
      }

      // Unpreset result listening
      template <typename Function>
      requires IsValidFunction<Function, bool, EventParameters...>
      inline static geode::ListenerHandle listenWithID(const char* id, Function&& function, int priority = 0) {
        if constexpr (std::is_void_v<EventReturn>)
          return Event(id).listen(std::forward<Function>(function), priority);
        else
          // the underlying event will receive the return value pointer, but will do nothing with it
          return Event(id).listen(
            [func = std::forward<Function>(function)](EventReturn*, EventParameters... args) { return func(args...); },
            priority
          );
      }

      // Preset result raw listening
      template <bool stop, typename Function>
      requires IsValidFunction<Function, void, EventReturnPointer, EventParameters...>
      inline static geode::ListenerHandle rawListenWithID(const char* id, Function&& function, int priority = 0) {
        if constexpr(std::is_void_v<EventReturn>)
          return Event(id).listen(
            [func = std::forward<Function>(function)](EventParameters... args) { func(nullptr, args...); return stop; },
            priority
          );
        else
          return Event(id).listen(
            [func = std::forward<Function>(function)](EventReturnPointer into, EventParameters... args) { func(into, args...); return stop; },
            priority
          );
      }

      // Unpreset result raw listening
      template <typename Function>
      requires IsValidFunction<Function, bool, EventReturnPointer, EventParameters...>
      inline static geode::ListenerHandle rawListenWithID(const char* id, Function&& function, int priority = 0) {
        if constexpr(std::is_void_v<EventReturn>)
          return Event(id).listen(
            [func = std::forward<Function>(function)](EventParameters... args) { return func(nullptr, args...); },
            priority
          );
        else
          return Event(id).listen(
            [func = std::forward<Function>(function)](EventReturnPointer into, EventParameters... args) { return func(into, args...); },
            priority
          );
      }

      // Preset result sending
      template <bool stop, typename Function>
      requires IsValidFunction<Function, EventReturn, EventParameters...> && (!std::is_void_v<EventReturn>)
      inline static geode::ListenerHandle listenAndReturnWithID(const char* id, Function&& function, int priority = 0) {
        return Event(id).listen(
          [func = std::forward<Function>(function)](EventReturn* into, EventParameters... args) {
            if (into)
              *into = func(args...);
            else
              func(args...); 
            return stop; 
          },
          priority
        );
      }

      // Unpreset result sending
      template <typename Function>
      requires IsValidFunction<Function, std::pair<bool, EventReturn>, EventParameters...> && (!std::is_void_v<EventReturn>)
      inline static geode::ListenerHandle listenAndReturnWithID(const char* id, Function&& function, int priority = 0) {
        return Event(id).listen(
          [func = std::forward<Function>(function)](EventReturn* into, EventParameters... args) {
            bool stop;
            if (into)
              std::tie(stop, *into) = func(args...);
            else
              std::tie(stop, std::ignore) = func(args...);
            return stop;
          },
          priority
        );
      }

      /***********************************/
      /* Preset ID versions of listeners */
      /***********************************/

      // listenWithID<bool>
      template <bool stop, typename Function>
      requires IsValidFunction<Function, void, EventParameters...> && (!std::is_void_v<EventID>)
      static geode::ListenerHandle listen(Function&& function)
      { return listenWithID<stop, Function>(EventImpl::ID, std::forward<Function>(function)); }

      // listenWithID
      template <typename Function>
      requires IsValidFunction<Function, bool, EventParameters...> && (!std::is_void_v<EventID>)
      static geode::ListenerHandle listen(Function&& function)
      { return listenWithID<Function>(EventImpl::ID, std::forward<Function>(function)); }
      
      // rawListenWithID<bool>
      template <bool stop, typename Function>
      requires IsValidFunction<Function, void, EventReturnPointer, EventParameters...> && (!std::is_void_v<EventID>)
      static geode::ListenerHandle rawListen(Function&& function)
      { return rawListenWithID<stop, Function>(EventImpl::ID, std::forward<Function>(function)); }
      
      // rawListenWithID
      template <typename Function>
      requires IsValidFunction<Function, bool, EventReturnPointer, EventParameters...> && (!std::is_void_v<EventID>)
      static geode::ListenerHandle rawListen(Function&& function)
      { return rawListenWithID<Function>(EventImpl::ID, std::forward<Function>(function)); }

      // listenAndReturnWithID<bool>
      template <bool stop, typename Function>
      requires IsValidFunction<Function, EventReturn, EventParameters...> && (!std::is_void_v<EventID>) && (!std::is_void_v<EventReturn>)
      static geode::ListenerHandle listenAndReturn(Function&& function)
      { return listenAndReturnWithID<stop , Function>(EventImpl::ID, std::forward<Function>(function)); }
      
      // listenAndReturnWithID
      template <typename Function>
      requires IsValidFunction<Function, std::pair<bool, EventReturn>, EventParameters...> && (!std::is_void_v<EventID>) && (!std::is_void_v<EventReturn>)
      static geode::ListenerHandle listenAndReturn(Function&& function)
      { return listenAndReturnWithID<Function>(EventImpl::ID, std::forward<Function>(function)); }

    };

    template <typename T>
    struct is_tuple : std::false_type {};

    template <typename... Args>
    struct is_tuple<std::tuple<Args...>> : std::true_type {};

    template <typename EventID = void, typename EventParameters = std::tuple<>, typename EventReturn = void>
    struct EventBuilder : public EventImpl<EventID, EventParameters, EventReturn> {
    private:
      template <StringLiteral... Id>
      static constexpr auto makeIdType() {
        if constexpr (sizeof...(Id) == 0)
          return static_cast<EventBuilder<void, EventParameters, EventReturn>*>(nullptr);
        else if constexpr (sizeof...(Id) == 1)
          return static_cast<EventBuilder<std::integral_constant<const char*, Id.value>..., EventParameters, EventReturn>*>(nullptr);
        else
          static_assert(sizeof...(Id) <= 1, "id<> accepts at most one string literal");
      }

      template <typename T>
      struct takes_tuple_helper {
          static_assert(is_tuple<T>::value, "takes_tuple requires a std::tuple<Args...> type");
          using type = EventBuilder<EventID, T, EventReturn>;
      };

    public:
      EventBuilder() = delete;
      EventBuilder(const EventBuilder&) = delete;
      EventBuilder(EventBuilder&&) = delete;
      EventBuilder& operator=(const EventBuilder&) = delete;
      EventBuilder& operator=(EventBuilder&&) = delete;
      ~EventBuilder() = delete;

      // Allows you to set the dispatch event ID as part of the type itself, so you only have to write it once; now you can't misspell it.
      // You can remove the id via ::withID<>, in case you want to do that for some reason.
      template <StringLiteral... ID>
      using withID = std::remove_pointer_t<decltype(makeIdType<ID...>())>;

      // You pass in the types of the parameters that the event takes, in order, the same way you would on a geode::Dispatch event.
      template <typename... Parameters>
      using takes = EventBuilder<EventID, std::tuple<Parameters...>, EventReturn>;

      // Added in case you want to, say, copy a different EasyEvent's Takes values, you could do EasyEvent::takes_tuple<MyEvent::Takes>.
      // I could just pass ParameterTuple directly to EventBuilder, but the error messages suck, so we're doing it like this
      // since i can actually add a static assert into takes_tuple_helper.
      template <typename ParameterTuple = std::tuple<>>
      using takes_tuple = typename takes_tuple_helper<ParameterTuple>::type;

      // An EasyEvent may have a return value, passed via pointer, the underlying dispatch event receives that pointer as the first argument.
      // You can remove the return value (which is the same as setting it to void), by passing ::returns<> (or ::returns<void>).
      template <typename Return = void>
      using returns = EventBuilder<EventID, EventParameters, Return>;
    };

  }
  using EasyEvent = internal::EventBuilder<>;

}
