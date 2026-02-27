# Documentation

There is exactly one thing in the library that is exposed and meant to be used: `nytelyte::easy_event::EasyEvent`.

`EasyEvent` uses a builder pattern implemented with templates, allowing you to embed information about how your dispatch event is defined into the type.

## Creation

The available things that extend the `EasyEvent` are:

`EasyEvent::withID<"string literal">` - will embed the event ID into the event, when posting/listening, you do not have to include it anywhere in the arguments anymore  
`EasyEvent::takes<Types...>` - will set the parameters on the dispatch event, the same way you usually do that in Geode  
`EasyEvent::takes_tuple<std::tuple<Types...>>` - acts the same as `::takes<Types...>`, but accepts a single tuple instead, useful for, e.g. copying parameters from one event into another, if you need to do that for some reason  
`EasyEvent::returns<Type>` - sets the return type of the event, which is passed via pointer  

Additionally, blank values are supported:  
`EasyEvent::withID<>` - will remove the embedded ID from the event, if it has one  
`EasyEvent::returns<>` - equivalent to `EasyEvent::returns<void>`  
`EasyEvent::takes<>` - this means there are no arguments  
`EasyEvent::takes_tuple<>` - eqivalent to `EasyEvent::takes_tuple<std::tuple<>>`  

Do note that, obviously, this is is a builder for types, you aren't mutating anything when you use ::something, you're just referring to a new type.
```cpp
using MyEvent = EasyEvent::withID<"my id">;
// this doesn't clear the id from MyEvent, it just makes a new type from MyEvent, with the id cleared;
// the only thing you're achieving with this is getting a "declaration does not declare anything" warning
MyEvent::withID<>;
```

The types contained within an `EasyEvent` will be referred to as the `EventID`, `EventParameters...`, and the `EventReturn`.  
The `EventID` may be set or unset.  
The other ones have default values, when they are unset (`EventParameters...` = takes no arguments, `EventReturn` = does not return anything, identical to `void`).  
However, there is no default `EventID`. If it is unset, some methods are not available.  
Some methods are also not available if `EventReturn` is `void`.

## Utilities
###### Definitions
`Event` - an event which was created using the `EasyEvent` builder  
`Parameters...` - `Event`'s `EventParameters...`  
`Return` - `Event`'s `EventReturn`  
`ID` - `Event`'s `EventID`
###### Values
`Event::Event` - `geode::Dispatch<Parameters...>` if `Return` is void, otherwise `geode::Dispatch<Return*, Parameters...>`
`Event::ID` - `const char*`, the embedded `ID`'s value, or `nullptr` if `ID` is unset  
`Event::Returns` - `Return`  
`Event::Takes` - `std::tuple<Parameters...>`  

## Sending and Receiving

If you are familiar with Geode events (or if you just looked at the documentation), you might know that `Event::send()` only returns a `bool` (where `ListenerResult::Propagate` is `false` and `ListenerResult::Stop` is `true`), but we need a way to fetch a listener result if we need it, a return value if we need it, and both if we need both... So the sending side is split into 4 different method types:
- sending
- sending and receiving
- sending and receiving both
- raw sending

> [!NOTE]  
> Every single function below has a counterpart where you override the event ID, which must be used if the `EventID` is not set, and may or may not be used otherwise. They will not be documented for brevity, but they take an extra `const char *id` argument at the start, and their names end with `WithID`, e.g. `sendWithID`. They are otherwise identical.

#### Sending
###### Definitions
`Event` - an event which was created using the `EasyEvent` builder  
`Parameters...` - `Event`'s `EventParameters...`
###### Signature
`bool Event::send(Parameters... params);`  
###### Behavior
This will send the event with the given arguments.  
It will return the value that the underlying Geode dispatch event sending returns.  

#### Sending and receiving, requires `EventReturn` to be non-`void`
###### Defintions
`Event` - an event which was created using the `EasyEvent` builder  
`Parameters...` - `Event`'s `EventParameters...`
`Return` - `Event`'s `EventReturn`
###### Signature
`Return Event::sendAndReceive(Parameters... params);`  
###### Behavior
This will send the event with the given arguments.  
It will return the value sent to it by a listener through a pointer; if there were no listeners/none of them sent anything, the value is default constructed.  

#### Sending and receiving both, requires an event's `EventReturn` to be non-`void`
###### Definitions
`Event` - an event which was created using the `EasyEvent` builder  
`Parameters...` - `Event`'s `EventParameters...`  
`Return` - `Event`'s `EventReturn`
###### Signature
`std::pair<bool, Return> Event::sendAndReceiveBoth(Parameters... params);`  
###### Behavior
This will send the event with the given arguments.  
It will return the a pair containing return value that the underlying Geode dispatch event returns, and the value received from a listener through a pointer; if there were no listeners/none of them sent anything, the value is default constructed.

#### Raw sending
###### Definitions
`Event` - an event which was created using the `EasyEvent` builder  
`Parameters...` - `Event`'s `EventParameters...`  
`ReturnPointer` - if `Event`'s `EventReturn` is void, `std::nullptr_t`, otherwise `EventReturn*`  
###### Signature
`bool Event::rawSend(ReturnPointer into, Parameters... params);`  
###### Behavior
This will send the event with the given arguments.  
It will pass the `Return` pointer onto the event listener directly, as opposed to receiving, which default-constructs the object for you.  
If the return value is `void`, then it acts identically to `send`, albeit with an extra parameter that has to be `nullptr`.  
It will return the value that the underlying Geode dispatch event sending returns.  
Potential usecase of this is if your return type is not default-constructible.

## Listening/Sending

Standard Geode event listeners are forced to return `geode::ListenerResult`. We have to be able to easily send values, so the listening side is split into 3 different method types:
- listening
- listening and returning
- raw listening

> [!NOTE]  
> Every single function below has a counterpart where you override the event ID, which must be used if the `EventID` is not set, and may or may not be used otherwise. They will not be documented for brevity, but they take an extra `const char *id` argument at the start, and their names end with `WithID`, e.g. `listenWithID`. They are otherwise identical.

#### Listening
###### Defintions
`Event` - an event which was created using the `EasyEvent` builder  
`Parameters...` - `Event`'s `EventParameters...`  
`ListenerHandle` - `geode::comm::ListenerHandle`, the return type of `AnyEvent(...).listen(...)`
`Function` - a function which is of signature `bool(Parameters...)`  
`FunctionPreset` - a function which is of signature `void(Parameters...)`
###### Signatures
`ListenerHandle Event::listen(Function function, int priority = 0);`  
`ListenerHandle Event::listen<bool>(FunctionPreset functionPreset, int priority = 0);`   
###### Behavior
Creates an event listener with priority `priority` and returns it. The event listener does not care for the return type, if there is one; it completely ignores it.  
The underlying listener function acts like this:  
  - if a template argument was not provided to the function, it calls `function` and returns its result.
  - if a template argument was provided to the function, it calls `functionPreset` and returns the template argument provided.

#### Listening and returning, requires `EventReturn` to be non-`void`
###### Definitions
`Event` - an event which was created using the `EasyEvent` builder  
`Parameters...` - `Event`'s `EventParameters...`  
`Return` - `Event`'s `EventReturn`  
`ListenerHandle` - `geode::comm::ListenerHandle`, the return type of `AnyEvent(...).listen(...)`
`Function` - a function which is of signature `std::pair<geode::ListenerResult, Return>(Parameters...)`  
`FunctionPreset` - a function which is of signature `Return(Parameters...)`  
###### Signatures
`ListenerHandle Event::listenAndReturn(Function function, int priority = 0);`  
`ListenerHandle Event::listenAndReturn<bool>(FunctionPreset functionPreset, int priority = 0);`  
###### Behavior
Creates an event listener with priority `priority` and returns it.  
The underlying listener function acts like this:  
  - if a template argument was not provided to the function, it calls `function`, which returns a pair; the second value in the pair is stored into the return value pointer, the first one is returned by the listener
  - if a template argument was provided to the function, it calls `functionPreset`, which returns a value; the value is stored into the return value pointer, and the listener returns the provided template argument

#### Raw listening
###### Definitions
`Event` - an event which was created using the `EasyEvent` builder  
`Parameters...` - `Event`'s `EventParameters...`  
`ReturnPointer` - if `Event`'s `EventReturn` is void, `std::nullptr_t`, otherwise `EventReturn*`  
`ListenerHandle` - `geode::comm::ListenerHandle`, the return type of `AnyEvent(...).listen(...)`
`Function` - a function which is of signature `geode::ListenerResult(ReturnPointer, Parameters...)`  
`FunctionPreset` - a function which is of signature `void(ReturnPointer, arameters...)`  
###### Signatures
`ListenerHandle Event::rawListen(Function function, int priority = 0);`  
`ListenerHandle Event::rawListen<bool>(FunctionPreset functionPreset, int priority = 0);`  
###### Behavior
Creates an event listener with priority `priority` and returns it.  
The underlying listener function acts like this:  
  - if a template argument was not provided to the function, it calls `function` and returns its result.
  - if a template argument was provided to the function, it calls `functionPreset` and returns the template argument provided.

You are expected to manually deal with the return value pointer inside `function`/`functionPreset` however you wish, including:
  - storing a new value into it
  - modifying the value it points to, assuming you know that a different listener already stored something in it

The rule the other EasyEvent listeners follow is that if the return value pointer is `nullptr`, that means we are not intended to store anything into it. You should follow that as well, as regular sending passes `nullptr` as the return value pointer, expecting the listeners to handle that accordingly.  
