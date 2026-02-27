# EasyEvent

EasyEvent is a header-only library designed to make designing Geode event-based APIs easier and nicer to use.
This is meant to be an alternative to the event export macro.

## Event export macro

Straight from the Geode docs:
#### Header
```cpp
// (In your api distributed header file)
// (such as include/api.hpp)
#pragma once

#include <Geode/loader/Dispatch.hpp>
// You must **manually** declare the mod id, as macros like GEODE_MOD_ID will not
// behave correctly to other mods using your api.
#define MY_MOD_ID "dev.my-api"

namespace api {
    // Important: The function must be declared inline, and return a geode::Result,
    // as it can fail if the api is not available.
    inline geode::Result<int> addNumbers(int a, int b) GEODE_EVENT_EXPORT(&addNumbers, (a, b));
}
```
#### Source
```cpp
// MUST be defined before including the header.
#define GEODE_DEFINE_EVENT_EXPORTS
#include "../include/api.hpp"

Result<int> api::addNumbers(int a, int b) {
    return Ok(a + b);
}
```
Well, that looks pretty close to how you'd generally split your code into header and source, albeit with sligthly more wacky syntax, and the several caveats that the example explicitly mentions... Where even are the events?  
In reality, this will define events that grab function pointers, and call the function when the API function is called.

I personally find this clunky and ugly:
- you have to define a `MY_MOD_ID`, which makes sense, because the underlying events this creates will use the API's mod id, not yours
- you need to define a random `GEODE_DEFINE_EVENT_EXPORTS` before including an api header; this modifies the behavior of `#include <Geode/loader/Dispatch.hpp>`, specifically the `GEODE_EVENT_EXPORT` macro
- you have to write the function name twice in your header, and you have to pass the arguments to the macro as well; this is a limitation of macros

I feel like this is too much magic for a subpar result, aesthetically speaking. There are no functional issues with this, however, and thus no problems with you just sticking to it if you wish.
I also do not like that it hides the events from you.

## No event export macro

So, taking it for granted that we do not want to use the event export macros, how would we do something like this normally, without too much boilerplate?

#### Header
```cpp
namespace my_mod {
  // we make a dispatch event which contains one extra argument than we intend, a pointer to a return value
  // using MyEvent below, we model a function that takes two integers and adds them
  using MyEvent = geode::Dispatch<int*, int, int>;

  inline geode::Result<int> addNumbers(int a, int b) {
    if (!geode::Loader::get()->isModLoaded("your-mod-id")) return geode::Err("Mod not loaded");
    int result = 0;
    MyEvent("your-mod-id/your-event-id").send(&result, a, b);
    return geode::Ok(result);
  }
}
```
#### Source
```cpp
#include<path-to-your-header-where-you-have-the-event>

int addNumbers(int a, int b) { return a + b; }

$execute {

  my_mod::MyEvent("your-mod-id/your-event-id").listen(
    [](int *result, int a, int b) {
      *result = addNumbers(a, b);
      return ListenerResult::Propagate; // you could leave this out, it defaults to Propagate
    }
  ).leak();
  
};
```

I have two issues with this approach:
- you have to input the event ID twice, once in the header, and once in the source; you could work around this by defining it as a constant/macro in the header and using it, though some apis can get a lot of events, so you'd be defining a lot of constants
- you have to mess around with pointers manually to get a return value; this is not particularly difficult, but it gets irritating doing this for every API function
- the listener syntax makes it pretty hard to just pass an existing function, since we are forced to return a value convertible to boolean, and our method forces us to pass the pointer as the first argument: if your mod had an already existing addNumbers function that is (int, int) -> int, you couldn't just pass it directly to the listen method

## EasyEvent
To address those two issues, EasyEvent does three things:
- issue #1: you can pass the event id to an EasyEvent as a template parameter, meaning you only ever define it once; no need for a constant/macro or risking misspelling anything
- issue #2: support for events "with return values"
- issue #3: function overloads for common operations, such as return values, which makes passing our (int, int) -> int function possible

#### Header
```cpp
namespace my_mod {
  // we make a dispatch event which contains one extra argument than we intend, a pointer to a return value
  // using MyEvent below, we model a function that takes two integers and adds them
  using MyEvent = EasyEvent::withID<"your-mod-id/your-event-id">::takes<int, int>::returns<int>;

  inline geode::Result<int> addNumbers(int a, int b) {
    if (!geode::Loader::get()->isModLoaded("your-mod-id")) return geode::Err("Mod not loaded");
    return geode::Ok(MyEvent::sendAndReceive(a, b));
  }
}
```
#### Source
```cpp
#include<path-to-your-header-where-you-have-the-event>

int addNumbers(int a, int b) { return a + b; }

$execute {
  my_mod::MyEvent::listenAndReturn<ListenerResult::Propagate>(addNumbers).leak();
};
```

Do note that, under the hood, the "return values" are implemented the same way as my example which worked directly with dispatch events, with pointers, you just don't have to look at that.

To me, this reads a lot better than event exports, plus you still get fine-grained control over the events themselves, as this implementation does not just hide them from you; more in the [documentation](DOCS.md).

## How to use

Copy the header file into your mod, include it. Make sure to expose it in your API if you are using it there, and you probably are, since that is the point.  
Also make sure you compile with Clang. There is an internal compiler error on MSVC. If you know how to work around it without changing the interface, please open a pull request.

This software, readme (excluding the examples copied directly from the Geode docs), and documentation is released into the public domain.

There is also a testing file `event-tester.cpp`, which instantiates every template function to make sure it compiles. You may use it as an example, but a better example mod may be added soon; for now, check out [Icon Kit Filter & Sort](https://github.com/nytelytee/geode-icon-kit-filter-and-sort), a mod of mine which uses the EasyEvent system.
