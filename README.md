# EasyEvent

EasyEvent is a header-only library designed to make designing Geode event-based APIs easier and less error-prone.

## Events
The main problem with making an event-based API in Geode is how much boilerplate defining events requires.
Take the example from the Geode documentation:
#### Creation
```cpp
// DragDropEvent.hpp

#include <Geode/loader/Event.hpp> // Event
#include <Geode/cocos/cocoa/CCGeometry.h> // CCPoint

#include <vector> // std::vector
#include <filesystem> // std::filesystem::path

using namespace geode::prelude;

class DragDropEvent : public Event {
protected:
    std::vector<std::filesystem::path> m_files;
    CCPoint m_location;

public:
    DragDropEvent(std::vector<std::filesystem::path> const& files, CCPoint const& location);

    std::vector<std::filesystem::path> getFiles() const;
    CCPoint getLocation() const;
};
```
#### Posting
```cpp
// Assume those variables actually have useful values
std::vector<std::filesystem::path> files;
CCPoint location = CCPoint { 0.0f, 0.0f };

DragDropEvent(files, location).post();
```
#### Listening
```cpp
// main.cpp

#include <Geode/DefaultInclude.hpp> // $execute
#include <Geode/loader/Event.hpp> // EventListener, EventFilter

#include "DragDropEvent.hpp" // Our created event

using namespace geode::prelude;

// Execute runs the code inside **when your mod is loaded**
$execute {
    // This technically doesn't leak memory, since the listener should live for the entirety of the program
    new EventListener<EventFilter<DragDropEvent>>(+[](DragDropEvent* ev) {
        for (std::filesystem::path& file : ev->getFiles()) {
            log::debug("File dropped: {}", file);

            // ... handle the files here
        }

        // We have to propagate the event further, so that other listeners
        // can handle this event
        return ListenerResult::Propagate;
    });
}
```
## Dispatch events
That's a lot of code if all you need to do is expose some functions in your API...
Luckily, Geode itself already has a way to address this: dispatch events.
Dispatch events exist to reduce boilerplate when all you need is a simple event.  
Let's look at how they're used (also from the Geode documentation):

#### Creation
```cpp
using DragDropEvent = geode::DispatchEvent<ghc::filesystem::path>;
using DragDropFilter = geode::DispatchFilter<ghc::filesystem::path>;
```
#### Posting
```cpp
DragDropEvent("geode.drag-drop/default", "path/to/file").post();
```
#### Listening
```cpp
$execute {
    new EventListener(+[](ghc::filesystem::path const& path) {
        log::info("File dropped: {}", path);
        return ListenerResult::Propagate;
    }, DragDropFilter("geode.drag-drop/default"));
};
```

## My amazing event(s)
Well, that's certainly better. Let me try defining my own amazing event using Geode's dispatch event system!
#### Creation
```cpp
using MyAmazingFilter = DispatchFilter<int>;
// shorthand so you don't have to type the arguments twice, the Geode docs don't talk about this
using MyAmazingEvent = MyAmazingFilter::Event;
```
#### Posting
```cpp
MyAmazingEvent("nytelyte.example/my-amazing-event", 5);
```
#### Listening
```cpp
$execute {
  new EventListener(+[](int x) {
    log::info("You sent: {}. Amazing.", x);
    return ListenerResult::Propagate;
  }, DragDropFilter("nytelyte.example/my-amazing-even"));
};
```

Okay, that compiles fine without any warnings, let's push whatever button sends the event, check the logs and...  

Nothing happened.  
There were no errors, no crashes, just... **nothing happened**.  

Okay, what did I do wrong? Let me check my code.  
Okay, the creation looks fine, you can't exactly mess up there.  

Oh. I forgot to put `.post()` after the event. I did not get any warnings from my compiler, it just constructed the event object and then did nothing with it.  

Minor hiccup, let's do this again:
```cpp
MyAmazingEvent("nytelyte.example/my-amazing-event", 5).post();
```
Now I actually posted the event; surely, it's going to work now.

Now, let us press our amazing button that (actually) posts our amazing event, check the logs and...

Nothing happened.  
Okay, are you serious? What am I doing wrong now? The creating and posting is fine. Let me check the listening part.  

Oh. I misspelled the event ID. I forgot a 't' at the end.  
Wait, **why** exactly do I need to specify this in two different places?

At least in the listening side, you can use the `_spr` user-defined literal so you don't have to type out your mod's ID every time. But... you can't exactly do that on the posting side, can you? The parts that post your events typically go into an API header that other mod developers will be including. There, `_spr` would expand the event IDs to contain the mod ID which is including your API. You certainly don't want that. Best not forget about that either.  

Well... that sucks. I guess I just have to not make any mistakes that the compiler won't warn me about because all of that was perfectly valid C++ code. If you are making an API that requires a fair amount of events, good luck having to go through this whole dance for every single feature you want to add to your API.

This could, of course, just be a skill issue on my part. Maybe you don't have these issues, or you have tools that address them. But I still hold that it should be **impossible** to make these types of mistakes.

## What about functions that actually return things? 
Well that's for the errors (or lack thereof), but what about the features? Geode event listeners can't have return values, they just return a listener result to signal to the event system what it needs to do further. What if I want my API to call functions that have return values? Well, here's how you could do it (no intentional mistakes this time, promise):
#### Creation
```cpp
using MyAmazingFilter = DispatchFilter<int*, int>;
using MyAmazingEvent = MyAmazingFilter::Event;
```
#### Posting
```cpp
// in your api, this piece of code would typically be put inside a function, so that calling that function looks like just calling something that returns the result directly

int result = 0;
MyAmazingEvent("nytelyte.example/my-amazing-event", &result, 5).post();
// you now have access to whatever result the event listener decides to store into the result address, through result
```
#### Listening
```cpp
$execute {
  new EventListener(+[](int* result, int x) {
    *result = x + 1;
    return ListenerResult::Propagate;
  }, DragDropFilter("nytelyte.example/my-amazing-event"));
};
```

Well, I now have to mess around with pointers if I want a return value? Really? I mean, pointers are not complicated sure, but this is still boilerplate; I will have to do this for every event that returns a result; the same pattern, over and over.

## Event exports
Okay, does Geode have anything else that helps me deal with these issues?

Well, we have Geode event exports (again, straight from the Geode docs):
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

This is a bit better. It **is** harder to make mistakes (albeit not impossible), but uh...

## Why do I have to do this?
Let me be real, I just really don't like that users have to use this macro-heavy syntax and define things in the preprocessor to magically change the behavior of the code. I also dislike the fact that you have to duplicate the function name, and the function arguments too (because that second macro argument there is you specifying how the event export system should be calling your function).

I want something that has just enough "magic" to help you not make mistakes that the compiler can't catch, but also not crazy-looking, like event exports. Note that event exports really are fine if you want something that *works* and don't care as much as me about how it *looks*, go ahead and use them if you saw me describe them and decided that, unlike me, you don't see anything "wrong" with them. They do fix a lot of the issues with dispatch events that I was talking about: the ID is nowhere to be seen (although you can use a custom one, you still only define it once) and thus you don't really have to duplicate it anywhere, you can't accidentally forget to post the event, because you just are not interacting with the underlying Geode events themselves when writing your API, and they have return values built in, you don't have to manually mess around with pointers.

These are all good features, but I am not a fan of the way in which I have to make use of them.

I am willing to concede that this entirely a me problem and that no one besides me cares about any of this, but I decided to write this up and write my idea of a fix anyway.

## What I want

But what if we could, with just a *little bit* of template magic, do something like this:
#### Creation
```cpp
using MyAmazingEvent = EasyEvent::id<"nytelyte.example/my-amazing-event">::takes<int>;
using MyAmazingEventWithAReturnValue = EasyEvent::id<"nytelyte.example/my-amazing-event-with-a-return-value">::takes<int>::returns<int>;
```
#### Posting
```cpp
MyAmazingEvent::post(5);
int result = MyAmazingEventWithAReturnValue::receive(5);
```
#### Listening
```cpp
MyAmazingEvent::listen<ListenerResult::Propagate>(+[](int x){ log::info("You sent: {}. Amazing.", x); });
MyAmazingEvent::send<ListenerResult::Propagate>(+[](int x){ return x + 1; });
```

Let's see...  
The ID is defined only once, so you can't misspell it; check.  
You can't accidentally not post the event and not notice it, because it's a static method on the EasyEvent type; check.  
You get return values from functions without having to manually mess around with pointers repeatedly, for every API method; check.

Do note that, under the hood, the "return values" are implemented the same way as my example which worked directly with dispatch events, with pointers, you just don't have to look at that.

To me, this reads a lot better than event exports, plus you still get fine-grained control over the events themselves; more in the [documentation](DOCS.md).

## How to use

Copy the header file (preferably the one from releases, as the one in the repo uses a lot of macros; the release version selectively expands those macros to make the code less ugly and to improve error messages; the generator script is also included in this repository, under the name `generate.sh`) into your mod, include it. Make sure to expose it in your API if you are using it there, and you probably are, since that is the point.  
Also make sure you compile with Clang. There is an internal compiler error on MSVC. If you know how to work around it without changing the interface, please open a pull request.

This software, readme (excluding the examples copied directly from the Geode docs), and documentation is released into the public domain.
