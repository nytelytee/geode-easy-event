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

    template <typename Class, typename ReturnType, typename... EventParameters>
    using MemberFunction = typename geode::to_member<Class, ReturnType(EventParameters...)>::value;

    template <typename Class, typename ReturnType, typename... EventParameters>
    concept MemberFunctionHasValidReturnType = (
      (std::is_void_v<ReturnType> && std::is_void_v<std::invoke_result_t<MemberFunction<Class, ReturnType, EventParameters...>, Class*, EventParameters...>>) ||
      std::assignable_from<ReturnType&, std::invoke_result_t<MemberFunction<Class, ReturnType, EventParameters...>, Class*, EventParameters...>>
    );

    template <typename Class, typename ReturnType, typename... EventParameters>
    concept IsValidMemberFunction = MemberFunctionHasValidReturnType<Class, ReturnType, EventParameters...>;

    template <typename EventID, typename EventParameters, typename EventReturn>
    struct EventImpl;

    template <typename EventID, typename... EventParameters, typename EventReturn>
    struct EventImpl<EventID, std::tuple<EventParameters...>, EventReturn> {
    private:
      using EventReturnPointer = std::conditional_t<std::is_void_v<EventReturn>, std::nullptr_t, EventReturn*>;
    public:
      // Actual internal dispatch event values, in case you need them
      using Filter = std::conditional_t<
          std::is_void_v<EventReturn>,
          geode::DispatchFilter<EventParameters...>,
          geode::DispatchFilter<EventReturn*, EventParameters...>
      >;
      using Event = Filter::Event;
      using Listener = geode::EventListener<Filter>;

      // The parameters the EasyEvent is using right now, in case you need them
      static constexpr const char* ID = []{ if constexpr (std::is_void_v<EventID>) return nullptr; else return EventID::value; }();
      using Returns = EventReturn;
      using Takes = std::tuple<EventParameters...>;

      // Normal post
      inline static geode::ListenerResult postWithID(const char* id, EventParameters... args) {
        if constexpr (std::is_void_v<EventReturn>) return Event(id, args...).post();
        else return Event(id, nullptr, args...).post();
      }
      
      // Raw post
      inline static geode::ListenerResult rawPostWithID(const char* id, EventReturnPointer into, EventParameters... args) {
        if constexpr (std::is_void_v<EventReturn>) return Event(id, args...).post();
        else return Event(id, into, args...).post();
      }

      // Receiving post
      inline static EventReturn receiveWithID(const char* id, EventParameters... args)
      requires (!std::is_void_v<EventReturn>)  {
        EventReturn into;
        Event(id, &into, args...).post();
        return into;
      }

      // Receiving both post
      inline static std::pair<geode::ListenerResult, EventReturn> receiveBothWithID(const char* id, EventParameters... args)
      requires (!std::is_void_v<EventReturn>) {
        EventReturn into;
        geode::ListenerResult result = Event(id, &into, args...).post();
        return std::pair{result, into};
      }

      // If the easy event has an embedded ID, these post/receive/receiveBoth/rawPost overloads are defined, allowing you to post the event without an ID
      // This part is a lot more complicated for listeners given how many options they have to support, but luckily, at least this is simple enough to write
      // out manually.
      inline static geode::ListenerResult post(EventParameters... args)
      requires (!std::is_void_v<EventID>)
      { return postWithID(EventImpl::ID, args...); }
      
      inline static geode::ListenerResult rawPost(EventReturnPointer into, EventParameters... args)
      requires (!std::is_void_v<EventID>)
      { return rawPostWithID(EventImpl::ID, into, args...); }

      inline static EventReturn receive(EventParameters... args)
      requires (!std::is_void_v<EventID> && !std::is_void_v<EventReturn>)
      { return receiveWithID(EventImpl::ID, args...); }

      inline static std::pair<geode::ListenerResult, EventReturn> receiveBoth(EventParameters... args)
      requires (!std::is_void_v<EventID> && !std::is_void_v<EventReturn>)
      { return receiveBothWithID(EventImpl::ID, args...); }

      // Base functions

      // Preset result listening
      template <geode::ListenerResult result, typename Function>
      requires IsValidFunction<Function, void, EventParameters...>
      inline static Listener listenWithID(const char* id, Function&& function) {
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

      // Unpreset result listening
      template <typename Function>
      requires IsValidFunction<Function, geode::ListenerResult, EventParameters...>
      inline static Listener listenWithID(const char* id, Function&& function) {
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

      // Preset result raw listening
      template <geode::ListenerResult result, typename Function>
      requires IsValidFunction<Function, void, EventReturnPointer, EventParameters...>
      inline static Listener rawListenWithID(const char* id, Function&& function) {
        if constexpr(std::is_void_v<EventReturn>)
          return Listener(
            [func = std::forward<Function>(function)](EventParameters... args) {
              func(nullptr, args...);
              return result;
            },
            Filter(id)
          );
        else
          return Listener(
            [func = std::forward<Function>(function)](EventReturnPointer into, EventParameters... args) {
              func(into, args...);
              return result;
            },
            Filter(id)
          );
      }

      // Unpreset result raw listening
      template <typename Function>
      requires IsValidFunction<Function, geode::ListenerResult, EventReturnPointer, EventParameters...>
      inline static Listener rawListenWithID(const char* id, Function&& function) {
        if constexpr(std::is_void_v<EventReturn>)
          return Listener(
            [func = std::forward<Function>(function)](EventParameters... args) {
              return func(nullptr, args...);
            },
            Filter(id)
          );
        else
          return Listener(
            [func = std::forward<Function>(function)](EventReturnPointer into, EventParameters... args) {
              return func(into, args...);
            },
            Filter(id)
          );
      }


      // Preset result sending
      template <geode::ListenerResult result, typename Function>
      requires IsValidFunction<Function, EventReturn, EventParameters...> && (!std::is_void_v<EventReturn>)
      inline static Listener sendWithID(const char* id, Function&& function) {
        return Listener(
          [func = std::forward<Function>(function)](EventReturn* into, EventParameters... args) {
            if (into) *into = func(args...);
            else func(args...);
            return result;
          },
          Filter(id)
        );
      }

      // Unpreset result sending
      template <typename Function>
      requires IsValidFunction<Function, std::pair<geode::ListenerResult, EventReturn>, EventParameters...> && (!std::is_void_v<EventReturn>)
      inline static Listener sendWithID(const char* id, Function&& function) {
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

      // Base member functions, suffixed with On

      // Preset result listening
      template <geode::ListenerResult result, typename Class>
      requires IsValidMemberFunction<Class, void, EventParameters...>
      inline static Listener listenWithIDOn(const char* id, Class* instance, MemberFunction<Class, void, EventParameters...> function) {
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

      // Unpreset result listening
      template <typename Class>
      requires IsValidMemberFunction<Class, geode::ListenerResult, EventParameters...>
      inline static Listener listenWithIDOn(const char* id, Class* instance, MemberFunction<Class, geode::ListenerResult, EventParameters...> function) {
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

      // Preset result raw listening
      template <geode::ListenerResult result, typename Class>
      requires IsValidMemberFunction<Class, void, EventReturnPointer, EventParameters...>
      inline static Listener rawListenWithIDOn(const char* id, Class* instance, MemberFunction<Class, void, EventReturnPointer, EventParameters...> function) {
        if constexpr (std::is_void_v<EventReturn>)
          return Listener(
            [instance, function](EventParameters... args) {
              (instance->*function)(nullptr, args...);
              return result;
            },
            Filter(id)
          );
        else
          return Listener(
            [instance, function](EventReturnPointer into, EventParameters... args) {
              (instance->*function)(into, args...);
              return result;
            },
            Filter(id)
          );
      }

      // Unpreset result raw listening
      template <typename Class>
      requires IsValidMemberFunction<Class, geode::ListenerResult, EventReturnPointer, EventParameters...>
      inline static Listener rawListenWithIDOn(const char* id, Class* instance, MemberFunction<Class, geode::ListenerResult, EventReturnPointer, EventParameters...> function) {
        if constexpr(std::is_void_v<EventReturn>)
          return Listener(
            [instance, function](EventParameters... args) {
              return (instance->*function)(nullptr, args...);
            },
            Filter(id)
          );
        else
          return Listener(
            [instance, function](EventReturnPointer into, EventParameters... args) {
              return (instance->*function)(into, args...);
            },
            Filter(id)
          );
      }

      // Preset result sending
      template <geode::ListenerResult result, typename Class>
      requires IsValidMemberFunction<Class, EventReturn, EventParameters...> && (!std::is_void_v<EventReturn>)
      inline static Listener sendWithIDOn(const char* id, Class* instance, MemberFunction<Class, EventReturn, EventParameters...> function) {
        return Listener(
          [instance, function](EventReturn* into, EventParameters... args) {
            if (into) *into = (instance->*function)(args...);
            else (instance->*function)(args...);
            return result;
          },
          Filter(id)
        );
      }

      // Unpreset result sending
      template <typename Class>
      requires IsValidMemberFunction<Class, std::pair<geode::ListenerResult, EventReturn>, EventParameters...> && (!std::is_void_v<EventReturn>)
      inline static Listener sendWithIDOn(const char* id, Class* instance, MemberFunction<Class, std::pair<geode::ListenerResult, EventReturn>, EventParameters...> function) {
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

      /**** START MACRO SEQUENCE ****/
      #pragma push_macro("EE_NL")
      #ifndef EASY_EVENT_PRETTIFY_MACROS_IS_THIS_NAME_LONG_ENOUGH_TO_NOT_ACCIDENTALLY_BE_DEFINED_BY_SOMEONE_ELSE_YET
      #define EE_NL
      #else
      #undef EE_NL
      #endif

      #ifndef EASY_EVENT_PRETTIFY_MACROS_IS_THIS_NAME_LONG_ENOUGH_TO_NOT_ACCIDENTALLY_BE_DEFINED_BY_SOMEONE_ELSE_YET
      // WARNING: Horrifying macro code ahead, good luck trying to understand this;
      //          I need to define like 36 overloads for functions, which may have different names,
      //          parameters, and template parameters. They are all extremely simple functions that can be
      //          defined in terms of the above WithID functions, however.
      // NOTE: Use the version of the file without these macros, generated from this file using a shell script also
      //       inside of this repository. The generated file is in releases, even though you could just
      //       run the script yourself. You are also able to use this file if you so wish, as I am ensuring that using it
      //       is also safe:
      //         - every macro is pushed and then popped, so I can't accidentally redefine someone else's macros
      //           (the one exception is the macro that is used to preprocess this part of the file)
      //         - EE_NL is defined (blank) if the prettifying macro is not defined, so it has no effect
      //           (also pushed and popped in case someone else defines EE_NL)
      //       I would, however, recommend using the version in releases, as error messages from defined macros are horrible.
      //       This only exists so I can reproducibly create simple function overloads to avoid making mistakes.
      #else
      /**********************************************************************************************************/
      /******************************************THIS PART IS AUTOGENERATED**************************************/
      /**********************************************************************************************************/ EE_NL
      #endif

      #pragma push_macro("UNBRACKET_NO_COMMA")
      #pragma push_macro("UNBRACKET_COMMA")
      #pragma push_macro("UNBRACKET_REQUIRES_CLAUSE")
      #pragma push_macro("CREATE_LISTENER")
      #pragma push_macro("CREATE_MEMBER_LISTENER")
      #pragma push_macro("CREATE_MAIN_GLOBAL_LISTENERS")
      #pragma push_macro("CREATE_MAIN_GLOBAL_RAW_LISTENERS")
      #pragma push_macro("CREATE_LISTENERS_WITH_PRESET_EVENT_ID")
      #pragma push_macro("CREATE_RAW_LISTENERS_WITH_PRESET_EVENT_ID")

      #define UNBRACKET_NO_COMMA(...) __VA_ARGS__
      #define UNBRACKET_COMMA(...) __VA_ARGS__ __VA_OPT__(,)
      #define UNBRACKET_REQUIRES_CLAUSE(...) __VA_OPT__(&&) __VA_ARGS__

      #define CREATE_LISTENER(\
        ExtraTemplateParameter, /* shall either be "(geode::ListenerResult result)" or "()" */ \
        BaseReturnType, /* shall be non-blank and bracketed" */ \
        BaseParameters, /* shall either be "(EventParameters...)" or "(EventReturnPointer, EventParameters...)" */ \
        ListenerReturnType, /* shall either be "Listener" or "Listener*" */ \
        Name, /* shall be non-blank */ \
        ExtraFunctionParameter, /* shall either be "(const char* id)" or "()" */ \
        ExtraRequiresClause, /* shall be bracketed */ \
        ReturnPrefix, /* shall either be "new Listener" or "" */ \
        BaseName, /* shall be non-blank */ \
        ExtraTemplateArgument, /* shall either be "(result)" or "()" */\
        ExtraFunctionArgument /* shall either be "id" or "EventImpl::ID" */ \
      ) EE_NL\
        template <UNBRACKET_COMMA ExtraTemplateParameter typename Function> EE_NL\
        requires IsValidFunction<Function, UNBRACKET_NO_COMMA BaseReturnType, UNBRACKET_NO_COMMA BaseParameters> UNBRACKET_REQUIRES_CLAUSE ExtraRequiresClause EE_NL\
        static ListenerReturnType Name(UNBRACKET_COMMA ExtraFunctionParameter Function&& function) EE_NL\
        { return ReturnPrefix (BaseName<UNBRACKET_COMMA ExtraTemplateArgument Function>(ExtraFunctionArgument, std::forward<Function>(function))); } EE_NL

      #define CREATE_MEMBER_LISTENER(\
        ExtraTemplateParameter, /* shall either be "(geode::ListenerResult result)" or "()" */ \
        BaseReturnType, /* shall be non-blank and bracketed */ \
        BaseParameters, /* shall either be "(EventParameters...)" or "(EventReturnPointer, EventParameters...)" */ \
        ListenerReturnType, /* shall either be "Listener" or "Listener*" */ \
        Name, /* shall be non-blank */ \
        ExtraFunctionParameter, /* shall either be "(const char* id)" or "()" */ \
        ExtraRequiresClause, /* shall be bracketed */ \
        ReturnPrefix, /* shall either be "new Listener" or "" */ \
        BaseName, /* shall be non-blank */ \
        ExtraTemplateArgument, /* shall either be "(result)" or "()" */\
        ExtraFunctionArgument /* shall either be "id" or "EventImpl::ID" */ \
      ) EE_NL\
        template <UNBRACKET_COMMA ExtraTemplateParameter typename Class> EE_NL\
        requires IsValidMemberFunction<Class, UNBRACKET_NO_COMMA BaseReturnType, UNBRACKET_NO_COMMA BaseParameters> UNBRACKET_REQUIRES_CLAUSE ExtraRequiresClause EE_NL\
        static ListenerReturnType Name(UNBRACKET_COMMA ExtraFunctionParameter Class* instance, MemberFunction<Class, UNBRACKET_NO_COMMA BaseReturnType, UNBRACKET_NO_COMMA BaseParameters> function) EE_NL\
        { return ReturnPrefix (BaseName<UNBRACKET_COMMA ExtraTemplateArgument Class>(ExtraFunctionArgument, instance, function)); } EE_NL
      

      #define CREATE_MAIN_GLOBAL_LISTENERS(Name, name, ReturnTypeTemplated, ReturnTypeUntemplated, ExtraRequires)\
        CREATE_LISTENER((geode::ListenerResult result), ReturnTypeTemplated, (EventParameters...), Listener*, global##Name, (const char* id), ExtraRequires, new Listener, name, (result), id)\
        CREATE_LISTENER((), ReturnTypeUntemplated, (EventParameters...), Listener*, global##Name, (const char* id), ExtraRequires, new Listener, name, (), id)\
        CREATE_MEMBER_LISTENER((geode::ListenerResult result), ReturnTypeTemplated, (EventParameters...), Listener*, global##Name##On, (const char* id), ExtraRequires, new Listener, name##On, (result), id)\
        CREATE_MEMBER_LISTENER((), ReturnTypeUntemplated, (EventParameters...), Listener*, global##Name##On, (const char* id), ExtraRequires, new Listener, name##On, (), id)\
      
      #define CREATE_MAIN_GLOBAL_RAW_LISTENERS(Name, name, ReturnTypeTemplated, ReturnTypeUntemplated, ExtraRequires)\
        CREATE_LISTENER((geode::ListenerResult result), ReturnTypeTemplated, (EventReturnPointer, EventParameters...), Listener*, global##Name, (const char* id), ExtraRequires, new Listener, name, (result), id)\
        CREATE_LISTENER((), ReturnTypeUntemplated, (EventReturnPointer, EventParameters...), Listener*, global##Name, (const char* id), ExtraRequires, new Listener, name, (), id)\
        CREATE_MEMBER_LISTENER((geode::ListenerResult result), ReturnTypeTemplated, (EventReturnPointer, EventParameters...), Listener*, global##Name##On, (const char* id), ExtraRequires, new Listener, name##On, (result), id)\
        CREATE_MEMBER_LISTENER((), ReturnTypeUntemplated, (EventReturnPointer, EventParameters...), Listener*, global##Name##On, (const char* id), (), new Listener, name##On, (), id)\

      /*************************/
      /* MAIN GLOBAL LISTENERS */
      /*************************/ EE_NL
      // Global versions of listenWithID[On]
      CREATE_MAIN_GLOBAL_LISTENERS(ListenWithID, listenWithID, (void), (geode::ListenerResult), ()) EE_NL\
      // Global versions of rawListenWithID[On]
      CREATE_MAIN_GLOBAL_RAW_LISTENERS(RawListenWithID, rawListenWithID, (void), (geode::ListenerResult), ()) EE_NL\
      // Global versions of sendWithID[On]
      CREATE_MAIN_GLOBAL_LISTENERS(SendWithID, sendWithID, (EventReturn), (std::pair<geode::ListenerResult, EventReturn>), ((!std::is_void_v<EventReturn>)))

      #define CREATE_LISTENERS_WITH_PRESET_EVENT_ID(Name, name, ReturnTypeTemplated, ReturnTypeUntemplated, ExtraRequires)\
        CREATE_LISTENER((geode::ListenerResult result), ReturnTypeTemplated, (EventParameters...), Listener, name, (), ExtraRequires, , name##WithID, (result), EventImpl::ID)\
        CREATE_LISTENER((), ReturnTypeUntemplated, (EventParameters...), Listener, name, (), ExtraRequires, , name##WithID, (), EventImpl::ID)\
        CREATE_LISTENER((geode::ListenerResult result), ReturnTypeTemplated, (EventParameters...), Listener*, global##Name, (), ExtraRequires, , global##Name##WithID, (result), EventImpl::ID)\
        CREATE_LISTENER((), ReturnTypeUntemplated, (EventParameters...), Listener*, global##Name, (), ExtraRequires, , global##Name##WithID, (), EventImpl::ID)\
        CREATE_MEMBER_LISTENER((geode::ListenerResult result), ReturnTypeTemplated, (EventParameters...), Listener, name##On, (), ExtraRequires, , name##WithIDOn, (result), EventImpl::ID)\
        CREATE_MEMBER_LISTENER((), ReturnTypeUntemplated, (EventParameters...), Listener, name##On, (), ExtraRequires, , name##WithIDOn, (), EventImpl::ID)\
        CREATE_MEMBER_LISTENER((geode::ListenerResult result), ReturnTypeTemplated, (EventParameters...), Listener*, global##Name##On, (), ExtraRequires, , global##Name##WithIDOn, (result), EventImpl::ID)\
        CREATE_MEMBER_LISTENER((), ReturnTypeUntemplated, (EventParameters...), Listener*, global##Name##On, (), ExtraRequires, , global##Name##WithIDOn, (), EventImpl::ID)
      
      #define CREATE_RAW_LISTENERS_WITH_PRESET_EVENT_ID(Name, name, ReturnTypeTemplated, ReturnTypeUntemplated, ExtraRequires)\
        CREATE_LISTENER((geode::ListenerResult result), ReturnTypeTemplated, (EventReturnPointer, EventParameters...), Listener, name, (), ExtraRequires, , name##WithID, (result), EventImpl::ID)\
        CREATE_LISTENER((), ReturnTypeUntemplated, (EventReturnPointer, EventParameters...), Listener, name, (), ExtraRequires, , name##WithID, (), EventImpl::ID)\
        CREATE_LISTENER((geode::ListenerResult result), ReturnTypeTemplated, (EventReturnPointer, EventParameters...), Listener*, global##Name, (), ExtraRequires, , global##Name##WithID, (result), EventImpl::ID)\
        CREATE_LISTENER((), ReturnTypeUntemplated, (EventReturnPointer, EventParameters...), Listener*, global##Name, (), ExtraRequires, , global##Name##WithID, (), EventImpl::ID)\
        CREATE_MEMBER_LISTENER((geode::ListenerResult result), ReturnTypeTemplated, (EventReturnPointer, EventParameters...), Listener, name##On, (), ExtraRequires, , name##WithIDOn, (result), EventImpl::ID)\
        CREATE_MEMBER_LISTENER((), ReturnTypeUntemplated, (EventReturnPointer, EventParameters...), Listener, name##On, (), ExtraRequires, , name##WithIDOn, (), EventImpl::ID)\
        CREATE_MEMBER_LISTENER((geode::ListenerResult result), ReturnTypeTemplated, (EventReturnPointer, EventParameters...), Listener*, global##Name##On, (), ExtraRequires, , global##Name##WithIDOn, (result), EventImpl::ID)\
        CREATE_MEMBER_LISTENER((), ReturnTypeUntemplated, (EventReturnPointer, EventParameters...), Listener*, global##Name##On, (), ExtraRequires, , global##Name##WithIDOn, (), EventImpl::ID)


      /*****************************/
      /* PRESET EVENT ID LISTENERS */
      /*****************************/ EE_NL
      // Preset versions of listenWithID[On]/globalListenWithID[On]
      CREATE_LISTENERS_WITH_PRESET_EVENT_ID(Listen, listen, (void), (geode::ListenerResult), ((!std::is_void_v<EventID>))) EE_NL\
      // Preset versions of rawListenWithID[On]/globalRawListenWithID[On]
      CREATE_RAW_LISTENERS_WITH_PRESET_EVENT_ID(RawListen, rawListen, (void), (geode::ListenerResult), ((!std::is_void_v<EventID>))) EE_NL\
      // Preset versions of sendWithID[On]/globalSendWithID[On]
      CREATE_LISTENERS_WITH_PRESET_EVENT_ID(Send, send, (EventReturn), (std::pair<geode::ListenerResult, EventReturn>), ((!std::is_void_v<EventID>) && (!std::is_void_v<EventReturn>)))

      #pragma pop_macro("UNBRACKET_NO_COMMA")
      #pragma pop_macro("UNBRACKET_COMMA")
      #pragma pop_macro("UNBRACKET_REQUIRES_CLAUSE")
      #pragma pop_macro("CREATE_LISTENER")
      #pragma pop_macro("CREATE_MEMBER_LISTENER")
      #pragma pop_macro("CREATE_MAIN_GLOBAL_LISTENERS")
      #pragma pop_macro("CREATE_MAIN_GLOBAL_RAW_LISTENERS")
      #pragma pop_macro("CREATE_LISTENERS_WITH_PRESET_EVENT_ID")
      #pragma pop_macro("CREATE_RAW_LISTENERS_WITH_PRESET_EVENT_ID")

      #pragma pop_macro("EE_NL")
      /**** END MACRO SEQUENCE ****/
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
      // You can remove the id via ::id<>, in case you want to do that for some reason.
      template <StringLiteral... Id>
      using id = std::remove_pointer_t<decltype(makeIdType<Id...>())>;

      // You pass in the types of the parameters that the event takes, in order, the same way you would on a geode::DispatchEvent.
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
