# Documentation

There is exactly one thing in the library that is exposed and meant to be used: `nytelyte::easy_event::EasyEvent`.

`EasyEvent` uses a builder pattern implemented with templates, allowing you to embed information about how your dispatch event is defined into the type.

## Creation

The available things that extend the `EasyEvent` are:

`EasyEvent::id<"string literal">` - will embed the event ID into the event, when posting/listening, you do not have to include it anywhere in the arguments anymore  
`EasyEvent::takes<Types...>` - will set the parameters on the dispatch event, the same way you usually do that in Geode  
`EasyEvent::takes_tuple<std::tuple<Types...>>` - acts the same as `::takes<Types...>`, but accepts a single tuple instead, useful for, e.g. copying parameters from one event into another, if you need to do that for some reason  
`EasyEvent::returns<Type>` - sets the return type of the event, which is passed via pointer  

Additionally, blank values are supported:  
`EasyEvent::id<>` - will remove the embedded ID from the event, if it has one  
`EasyEvent::returns<>` - equivalent to `EasyEvent::returns<void>`  
`EasyEvent::takes<>` - this means there are no arguments  
`EasyEvent::takes_tuple<>` - eqivalent to `EasyEvent::takes_tuple<std::tuple<>>`  

Do note that, obviously, this is is a builder for types, you aren't mutating anything when you use ::something, you're just making a new type.
```cpp
using MyEvent = EasyEvent::id<"my id">;
// this doesn't clear the id from MyEvent, it just makes a new type from MyEvent, with the id cleared;
// the only thing you're achieving with this is getting a "declaration does not declare anything" warning
MyEvent::id<>;
```

>[!NOTE]
>The underlying event of `EasyEvent::takes<Params...>` is `geode::DispatchEvent<Params...>`.  
>The underlying event of `EasyEvent::takes<Params...>::returns<Return>` is `geode::DispatchEvent<Return*, Params...>`.  
>`::id<>`/`::id<"whatever">` will not affect this.

The types contained within an `EasyEvent` will be referred to as the `EventID`, `EventParameters...`, and the `EventReturn`. What each is should be obvious.  
The `EventID` may be set or unset.  
The other ones have default values, when they are unset (`EventParameters...` = takes no arguments, `EventReturn` = does not return anything, void).  
However, there is no default `EventID`. If it is unset, some method overloads may not be available, when they are available or not will be specified.  

## Posting/Receiving

If you are familiar with Geode events (or if you just looked at the documentation), you might know that `Event::post()` only returns a `geode::ListenerResult`, but we need a way to fetch a listener result if we need it, a return value if we need it, and both if we need both... So the posting side is split into 3 different methods:
- regular posting
- receiving
- receiving both

>[!NOTE]
>Every single function below has an overload where you may exclude the `id` parameter if `EventID` is set, so they shall not be documented for brevity.

#### Regular posting
Given some `Event`, which was created using the `EasyEvent` builder:

`geode::ListenerResult Event::post(const char* id, EventParameters... params);`  
This will post the event with the given ID and arguments.  
It will return the value that the underlying Geode dispatch event returns.  
It does not matter if `EventID` is set or not, this post always uses the provided ID.

#### Receiving, requires `EventReturn` to be non-`void`
Given some `Event`, which was created using the `EasyEvent` builder:

`EventReturn Event::receive(const char* id, EventParameters... params);`  
This will post the event with the given ID and arguments.  
It will return the value sent to it by a listener through a pointer; if there were no listeners/none of them sent anything, the value is default constructed.  
It does not matter if `EventID` is set or not, this post always uses the provided ID.

#### Receiving both, requires `EventReturn` to be non-`void`
Given some `Event`, which was created using the `EasyEvent` builder:

`std::pair<geode::ListenerResult, EventReturn> Event::receiveBoth(const char* id, EventParameters... params);`  
This will post the event with the given ID and arguments.  
It will return the a pair containing return value that the underlying Geode dispatch event returns, and the value sent to it by a listener through a pointer; if there were no listeners/none of them sent anything, the value is default constructed.
It does not matter if `EventID` is set or not, this post always uses the provided ID.

## Listening/Sending

Standard Geode event listeners are forced to return `geode::ListenerResult`. We have to be able to easily send values (we don't want to be made to mess with pointers manually, after all), the listening side is split into 4 different methods:
- unpreset result listening
- preset result listening
- unpreset result sending
- preset result sending

>[!NOTE]
>Every single function below has an overload where you may exclude the `id` parameter if `EventID` is set, so they shall not be documented for brevity.

#### Unpreset result listening
Given some `Event`, which was created using the `EasyEvent` builder:  
Given some `Function`, which is of signature `geode::ListenerResult(EventParameters...)`:  
Given some `Class`, and its respective `MemberFunction`, which is `geode::ListenerResult (Class::*)(EventParameters...)`:  
Given a `Listener`, which is the underlying Geode event listener type, for our `Event`:  

`Listener Event::listen(const char* id, Function function);`  
`Listener Event::listenOn(const char* id, Class* instance, MemberFunction function);`  
`Listener* Event::globalListen(const char* id, Function function);`  
`Listener* Event::globalListenOn(const char* id, Class* instance, MemberFunction function);`  

Creates an event listener and returns it. The event listener does not care for the return type, if there is one; it completely ignores it.  
The underlying function the event is created out of simply calls `function` and returns its return value.  
Additionally, if a member function is passed, along with the instance it should be called on, the same thing happens, it's just that `instance->*function` is called instead.  
If using a listener that starts with `global`, a heap-allocated pointer to a `Listener` is returned instead of a `Listener` object.  
It does not matter if `EventID` is set or not, this post always uses the provided ID.

#### Preset result listening
Given some `Event`, which was created using the `EasyEvent` builder:  
Given some `Function`, which is of signature `void(EventParameters...)`:  
Given some `Class`, and its respective `MemberFunction`, which is `void (Class::*)(EventParameters...)`:  
Given a `Listener`, which is the underlying Geode event listener type, for our `Event`:  
Given a `preset`, which you set yourself when calling, which is either `geode::ListenerResult::Propagate` or `geode::ListenerResult::Stop`:  

`Listener Event::listen<preset>(const char* id, Function function);`  
`Listener Event::listenOn<preset>(const char* id, Class* instance, MemberFunction function);`  
`Listener* Event::globalListen<preset>(const char* id, Function function);`  
`Listener* Event::globalListenOn<preset>(const char* id, Class* instance, MemberFunction function);`  

Creates an event listener and returns it. The event listener does not care for the return type, if there is one; it completely ignores it.  
The underlying function the event is created out of simply calls `function` and returns `preset`, allowing you to automatically propagate/stop an event without always needing to make a lambda that returns a listener result, you can just delegate to an existing function if that is all you would have been doing anyway: `Event::globalListen<ListenerResult::Propagate>(existingFunction);`  
Additionally, if a member function is passed, along with the instance it should be called on, the same thing happens, it's just that `instance->*function` is called instead.  
If using a listener that starts with `global`, a heap-allocated pointer to a `Listener` is returned instead of a `Listener` object.  
It does not matter if `EventID` is set or not, this post always uses the provided ID.

#### Unpreset result sending, requires `EventReturn` to be non-`void`
Given some `Event`, which was created using the `EasyEvent` builder:  
Given some `Function`, which is of signature `std::pair<geode::ListenerResult, EventReturn>(EventParameters...)`:  
Given some `Class`, and its respective `MemberFunction`, which is `std::pair<geode::ListenerResult, EventReturn> (Class::*)(EventParameters...)`:  
Given a `Listener`, which is the underlying Geode event listener type, for our `Event`:  

`Listener Event::send(const char* id, Function function);`  
`Listener Event::sendOn(const char* id, Class* instance, MemberFunction function);`  
`Listener* Event::globalSend(const char* id, Function function);`  
`Listener* Event::globalSendOn(const char* id, Class* instance, MemberFunction function);`  

Creates an event listener and returns it.  
The underlying function the event is created out of simply calls `function` that gives a pair, sets the return value to the second element of the pair, and returns the first element of the pair.  
Additionally, if a member function is passed, along with the instance it should be called on, the same thing happens, it's just that `instance->*function` is called instead.  
If using a listener that starts with `global`, a heap-allocated pointer to a `Listener` is returned instead of a `Listener` object.  
It does not matter if `EventID` is set or not, this post always uses the provided ID.

#### Preset result sending, requires `EventReturn` to be non-`void`
Given some `Event`, which was created using the `EasyEvent` builder:  
Given some `Function`, which is of signature `EventReturn(EventParameters...)`:  
Given some `Class`, and its respective `MemberFunction`, which is `EventReturn (Class::*)(EventParameters...)`:  
Given a `Listener`, which is the underlying Geode event listener type, for our `Event`:  
Given a `preset`, which you set yourself when calling, which is either `geode::ListenerResult::Propagate` or `geode::ListenerResult::Stop`:  

`Listener Event::send<preset>(const char* id, Function function);`  
`Listener Event::sendOn<preset>(const char* id, Class* instance, MemberFunction function);`  
`Listener* Event::globalSend<preset>(const char* id, Function function);`  
`Listener* Event::globalSendOn<preset>(const char* id, Class* instance, MemberFunction function);`  

Creates an event listener and returns it.  
The underlying function the event is created out of simply calls `function`, sets the return value to its return value, and returns `preset`, allowing you to automatically propagate/stop an event without always needing to make a lambda, same as listening.  
Additionally, if a member function is passed, along with the instance it should be called on, the same thing happens, it's just that `instance->*function` is called instead.  
If using a listener that starts with `global`, a heap-allocated pointer to a `Listener` is returned instead of a `Listener` object.  
It does not matter if `EventID` is set or not, this post always uses the provided ID.

## Utilities
Given some `Event`, which was created using the `EasyEvent` builder:  

`Event::Filter` is a `geode::DispatchFilter<EventParameters...>` if `EventReturn` is `void`, and `geode::DispatchFilter<EventReturn*, EventParameters...>` otherwise  
`Event::Event` is `Event::Filter::Event` (which is a `geode::DispatchEvent` specialization with the same template arguments as for `Event::Filter`)  
`Event::Listener` is `geode::EventListener<Event::Filter>` (which is the appropriate underlying event listener for the given filter above)  

`Event::ID` is a `const char*`, the embedded event ID, or `nullptr` if no ID was embedded  
`Event::Returns` is `EventReturn`  
`Event::Takes` is `std::tuple<EventParameters...>`  
