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
`Event::Filter` - `geode::DispatchFilter<Parameters...>` if `Return` is `void`, otherwise `geode::DispatchFilter<Return*, Parameters...>`    
`Event::Event` - `Event::Filter::Event` (which is `geode::DispatchEvent<Parameters...>`/`geode::DispatchEvent<Return*, Parameters...>`)  
`Event::Listener` - `geode::EventListener<Event::Filter>`  

`Event::ID` - `const char*`, the embedded `ID`'s value, or `nullptr` if `ID` is unset  
`Event::Returns` - `Return`  
`Event::Takes` - `std::tuple<Parameters...>`  

## Posting/Receiving

If you are familiar with Geode events (or if you just looked at the documentation), you might know that `Event::post()` only returns a `geode::ListenerResult`, but we need a way to fetch a listener result if we need it, a return value if we need it, and both if we need both... So the posting side is split into 4 different method types:
- regular posting
- receiving
- receiving both
- raw posting

> [!NOTE]  
> Every single function below has a counterpart where you exclude the `id` parameter if `EventID` is set; you just drop the `WithID` from the function name. They will not be documented for brevity.

#### Regular posting
###### Definitions
`Event` - an event which was created using the `EasyEvent` builder  
`Parameters...` - `Event`'s `EventParameters...`
###### Signature
`geode::ListenerResult Event::postWithID(const char* id, Parameters... params);`  
###### Behavior
This will post the event with the given ID and arguments.  
It will return the value that the underlying Geode dispatch event posting returns.  

#### Receiving, requires `EventReturn` to be non-`void`
###### Defintions
`Event` - an event which was created using the `EasyEvent` builder  
`Parameters...` - `Event`'s `EventParameters...`
`Return` - `Event`'s `EventReturn`
###### Signature
`Return Event::receiveWithID(const char* id, Parameters... params);`  
###### Behavior
This will post the event with the given ID and arguments.  
It will return the value sent to it by a listener through a pointer; if there were no listeners/none of them sent anything, the value is default constructed.  

#### Receiving both, requires an event's `EventReturn` to be non-`void`
###### Definitions
`Event` - an event which was created using the `EasyEvent` builder  
`Parameters...` - `Event`'s `EventParameters...`  
`Return` - `Event`'s `EventReturn`
###### Signature
`std::pair<geode::ListenerResult, Return> Event::receiveBothWithID(const char* id, Parameters... params);`  
###### Behavior
This will post the event with the given ID and arguments.  
It will return the a pair containing return value that the underlying Geode dispatch event returns, and the value sent to it by a listener through a pointer; if there were no listeners/none of them sent anything, the value is default constructed.

#### Raw posting
###### Definitions
`Event` - an event which was created using the `EasyEvent` builder  
`Parameters...` - `Event`'s `EventParameters...`  
`ReturnPointer` - if `Event`'s `EventReturn` is void, `std::nullptr_t`, otherwise `EventReturn*`  
###### Signature
`geode::ListenerResult Event::rawPostWithID(const char* id, ReturnPointer into, Parameters... params);`  
###### Behavior
This will post the event with the given ID and arguments.  
It will pass the `Return` pointer onto the event listener directly, as opposed to receiving, which default-constructs the object for you.  
If the return value is `void`, then it acts identically to `postWithID`, albeit with an extra parameter that has to be `nullptr`.  
It will return the value that the underlying Geode dispatch event posting returns.  
Potential usecase of this is if your return type is not default-constructible.

## Listening/Sending

Standard Geode event listeners are forced to return `geode::ListenerResult`. We have to be able to easily send values, so the listening side is split into 3 different method types:
- listening
- sending
- raw listening

> [!NOTE]  
> Every single function below has a counterpart where you exclude the `id` parameter if `EventID` is set; you just drop the `WithID` from the function name. They will not be documented for brevity.

#### Listening
###### Defintions
`Event` - an event which was created using the `EasyEvent` builder  
`Parameters...` - `Event`'s `EventParameters...`  
`Listener` - `Event::Listener`  
`Function` - a function which is of signature `geode::ListenerResult(Parameters...)`  
`FunctionPreset` - a function which is of signature `void(Parameters...)`
`Class` - a class  
`MemberFunction` - a pointer-to-member function which is of type `geode::ListenerResult (Class::*)(Parameters...)`  
`MemberFunctionPreset` - a pointer-to-member function which is of type `void (Class::*)(Parameters...)`      
###### Unpreset signatures
`Listener Event::listenWithID(const char* id, Function function);`  
`Listener Event::listenWithIDOn(const char* id, Class* instance, MemberFunction function);`  
`Listener* Event::globalListenWithID(const char* id, Function function);`  
`Listener* Event::globalListenWithIDOn(const char* id, Class* instance, MemberFunction function);`  
###### Preset signatures
`Listener Event::listenWithID<geode::ListenerResult>(const char* id, FunctionPreset functionPreset);`   
`Listener Event::listenWithIDOn<geode::ListenerResult>(const char* id, Class* instance, MemberFunctionPreset functionPreset);`  
`Listener* Event::globalListenWithID<geode::ListenerResult>(const char* id, FunctionPreset functionPreset);`  
`Listener* Event::globalListenWithIDOn<geode::ListenerResult>(const char* id, Class* instance, MemberFunctionPreset functionPreset);`  
###### Behavior
Creates an event listener and returns it. The event listener does not care for the return type, if there is one; it completely ignores it.  
The underlying listener function acts like this:  
  - if a template argument was not provided to the function, it calls `function` and returns its result.
  - if a template argument was provided to the function, it calls `functionPreset` and returns the template argument provided.

Additionally, if a member function is passed, along with the instance it should be called on, the same thing happens, it's just that `(instance->*function)`/`(instance->*functionPreset)` is called instead.  
If using a listener that starts with `global`, a heap-allocated pointer to a `Listener` is returned instead of a `Listener` object.  

#### Sending, requires `EventReturn` to be non-`void`
###### Definitions
`Event` - an event which was created using the `EasyEvent` builder  
`Parameters...` - `Event`'s `EventParameters...`  
`Return` - `Event`'s `EventReturn`  
`Listener` - `Event::Listener`  
`Function` - a function which is of signature `std::pair<geode::ListenerResult, Return>(Parameters...)`  
`FunctionPreset` - a function which is of signature `Return(Parameters...)`  
`Class` - a class  
`MemberFunction` - a pointer-to-member function which is of type `std::pair<geode::ListenerResult, Return> (Class::*)(Parameters...)`  
`MemberFunctionPreset` - a pointer-to-member function which is of type `Return (Class::*)(Parameters...)`  
###### Unpreset signatures
`Listener Event::sendWithID(const char* id, Function function);`  
`Listener Event::sendWithIDOn(const char* id, Class* instance, MemberFunction function);`  
`Listener* Event::globalSendWithID(const char* id, Function function);`  
`Listener* Event::globalSendWithIDOn(const char* id, Class* instance, MemberFunction function);`  
###### Preset signatures
`Listener Event::sendWithID<geode::ListenerResult>(const char* id, FunctionPreset functionPreset);`  
`Listener Event::sendWithIDOn<geode::ListenerResult>(const char* id, Class* instance, MemberFunctionPreset functionPreset);`  
`Listener* Event::globalSendWithID<geode::ListenerResult>(const char* id, FunctionPreset functionPreset);`  
`Listener* Event::globalSendWithIDOn<geode::ListenerResult>(const char* id, Class* instance, MemberFunctionPreset functionPreset);`  
###### Behavior
Creates an event listener and returns it.  
The underlying listener function acts like this:  
  - if a template argument was not provided to the function, it calls `function`, which returns a pair; the second value in the pair is stored into the return value pointer, the first one is returned by the listener
  - if a template argument was provided to the function, it calls `functionPreset`, which returns a value; the value is stored into the return value pointer, and the listener returns the provided template argument

Additionally, if a member function is passed, along with the instance it should be called on, the same thing happens, it's just that `(instance->*function)`/`(instance->*functionPreset)` is called instead.  
If using a listener that starts with `global`, a heap-allocated pointer to a `Listener` is returned instead of a `Listener` object.  

#### Raw listening
###### Definitions
`Event` - an event which was created using the `EasyEvent` builder  
`Parameters...` - `Event`'s `EventParameters...`  
`ReturnPointer` - if `Event`'s `EventReturn` is void, `std::nullptr_t`, otherwise `EventReturn*`  
`Listener` - `Event::Listener`  
`Function` - a function which is of signature `geode::ListenerResult(ReturnPointer, Parameters...)`  
`FunctionPreset` - a function which is of signature `void(ReturnPointer, arameters...)`  
`Class` - a class  
`MemberFunction` - a pointer-to-member function which is of type `geode::ListenerResult (Class::*)(ReturnPointer, Parameters...)`  
`MemberFunctionPreset` - a pointer-to-member function which is of type `void (Class::*)(ReturnPointer, Parameters...)`  
###### Unpreset signatures
`Listener Event::rawListenWithID(const char* id, Function function);`  
`Listener Event::rawListenWithIDOn(const char* id, Class* instance, MemberFunction function);`  
`Listener* Event::globalRawListenWithID(const char* id, Function function);`  
`Listener* Event::globalRawListenWithIDOn(const char* id, Class* instance, MemberFunction function);`  
###### Preset signatures
`Listener Event::rawListenWithID<geode::ListenerResult>(const char* id, FunctionPreset functionPreset);`  
`Listener Event::rawListenWithIDOn<geode::ListenerResult>(const char* id, Class* instance, MemberFunctionPreset functionPreset);`  
`Listener* Event::globalRawListenWithID<geode::ListenerResult>(const char* id, FunctionPreset functionPreset);`  
`Listener* Event::globalRawListenWithIDOn<geode::ListenerResult>(const char* id, Class* instance, MemberFunctionPreset functionPreset);`  
###### Behavior
Creates an event listener and returns it.  
The underlying listener function acts like this:  
  - if a template argument was not provided to the function, it calls `function` and returns its result.
  - if a template argument was provided to the function, it calls `functionPreset` and returns the template argument provided.

You are expected to manually deal with the return value pointer inside `function`/`functionPreset` however you wish, including:
  - storing a new value into it
  - modifying the value it points to, assuming you know that a different listener already stored something in it

The rule the other EasyEvent listeners follow is that if the return value pointer is `nullptr`, that means we are not intended to store anything into it. You should follow that as well, as regular posting passes `nullptr` as the return value pointer, expecting the listeners to handle that accordingly.  
Additionally, if a member function is passed, along with the instance it should be called on, the same thing happens, it's just that `(instance->*function)`/`(instance->*functionPreset)` is called instead.  
If using a listener that starts with `global`, a heap-allocated pointer to a `Listener` is returned instead of a `Listener` object.  
