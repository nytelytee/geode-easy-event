#include <api/easy-event.hpp>

using EasyEvent = nytelyte::easy_event::EasyEvent;

using MyEvent1 = EasyEvent::id<"my-event-id">::takes<int>;
using MyEvent2 = EasyEvent::id<"my-event-id">::takes<int>::returns<int>;

using MyEvent1NoID = EasyEvent::takes<int>;
using MyEvent2NoID = EasyEvent::takes<int>::returns<int>;

void test1(int) { return; }
int test2(int x) { return x; }
geode::ListenerResult test3(int) { return geode::ListenerResult::Propagate; }
std::pair<geode::ListenerResult, int> test4(int x) { return std::pair{geode::ListenerResult::Propagate, x}; }
void rawTest1(std::nullptr_t, int) { return; }
void rawTest2(int*, int) { return; }
geode::ListenerResult rawTest3(std::nullptr_t, int) { return geode::ListenerResult::Propagate; }
geode::ListenerResult rawTest4(int*, int) { return geode::ListenerResult::Propagate; }

struct Test {
  void test1(int) { return; }
  int test2(int x) { return x; }
  geode::ListenerResult test3(int) { return geode::ListenerResult::Propagate; }
  std::pair<geode::ListenerResult, int> test4(int x) { return std::pair{geode::ListenerResult::Propagate, x}; }
  void rawTest1(std::nullptr_t, int) { return; }
  void rawTest2(int*, int) { return; }
  geode::ListenerResult rawTest3(std::nullptr_t, int) { return geode::ListenerResult::Propagate; }
  geode::ListenerResult rawTest4(int*, int) { return geode::ListenerResult::Propagate; }
};

void testPosting() {
  geode::ListenerResult result;
  int returnValue;
  
  result = MyEvent1::post(5);
  result = MyEvent1::postWithID("my-event-id-overridden", 5);
  
  // this should not compile, return value is void, so there is nothing to return
  /*  
  returnValue = MyEvent1::receive(5);
  returnValue = MyEvent1::receiveWithID("my-event-id-overridden", 5);

  std::tie(result, returnValue) = MyEvent1::receiveBoth(5);
  std::tie(result, returnValue) = MyEvent1::receiveBothWithID("my-event-id-overridden", 5);
  */

  result = MyEvent1::rawPost(nullptr, 5);
  result = MyEvent1::rawPostWithID("my-event-id-overridden", nullptr, 5);
  
  result = MyEvent2::post(5);
  result = MyEvent2::postWithID("my-event-id-overridden", 5);

  returnValue = MyEvent2::receive(5);
  returnValue = MyEvent2::receiveWithID("my-event-id-overridden", 5);

  std::tie(result, returnValue) = MyEvent2::receiveBoth(5);
  std::tie(result, returnValue) = MyEvent2::receiveBothWithID("my-event-id-overridden", 5);

  result = MyEvent2::rawPost(&returnValue, 5);
  result = MyEvent2::rawPostWithID("my-event-id-overridden", &returnValue, 5);

}

void testListening() {

  Test testObject;
  Test* test = &testObject;

  MyEvent1::listen<geode::ListenerResult::Propagate>(test1);
  MyEvent1::listen(test3);
  MyEvent1::listenOn<geode::ListenerResult::Propagate>(test, &Test::test1);
  MyEvent1::listenOn(test, &Test::test3);
  MyEvent1::globalListen<geode::ListenerResult::Propagate>(test1);
  MyEvent1::globalListen(test3);
  MyEvent1::globalListenOn<geode::ListenerResult::Propagate>(test, &Test::test1);
  MyEvent1::globalListenOn(test, &Test::test3);

  MyEvent2::send<geode::ListenerResult::Propagate>(test2);
  MyEvent2::send(test4);
  MyEvent2::sendOn<geode::ListenerResult::Propagate>(test, &Test::test2);
  MyEvent2::sendOn(test, &Test::test4);
  MyEvent2::globalSend<geode::ListenerResult::Propagate>(test2);
  MyEvent2::globalSend(test4);
  MyEvent2::globalSendOn<geode::ListenerResult::Propagate>(test, &Test::test2);
  MyEvent2::globalSendOn(test, &Test::test4);

  MyEvent1::rawListen<geode::ListenerResult::Propagate>(rawTest1);
  MyEvent1::rawListen(rawTest3);
  MyEvent1::rawListenOn<geode::ListenerResult::Propagate>(test, &Test::rawTest1);
  MyEvent1::rawListenOn(test, &Test::rawTest3);
  MyEvent1::globalRawListen<geode::ListenerResult::Propagate>(rawTest1);
  MyEvent1::globalRawListen(rawTest3);
  MyEvent1::globalRawListenOn<geode::ListenerResult::Propagate>(test, &Test::rawTest1);
  MyEvent1::globalRawListenOn(test, &Test::rawTest3);

  MyEvent2::rawListen<geode::ListenerResult::Propagate>(rawTest2);
  MyEvent2::rawListen(rawTest4);
  MyEvent2::rawListenOn<geode::ListenerResult::Propagate>(test, &Test::rawTest2);
  MyEvent2::rawListenOn(test, &Test::rawTest4);
  MyEvent2::globalRawListen<geode::ListenerResult::Propagate>(rawTest2);
  MyEvent2::globalRawListen(rawTest4);
  MyEvent2::globalRawListenOn<geode::ListenerResult::Propagate>(test, &Test::rawTest2);
  MyEvent2::globalRawListenOn(test, &Test::rawTest4);

  // events with preset id allow for overriding it
  MyEvent1::listenWithID<geode::ListenerResult::Propagate>("my-event-id-overridden", test1);
  MyEvent1::listenWithID("my-event-id-overridden", test3);
  MyEvent1::listenWithIDOn<geode::ListenerResult::Propagate>("my-event-id-overridden", test, &Test::test1);
  MyEvent1::listenWithIDOn("my-event-id-overridden", test, &Test::test3);
  MyEvent1::globalListenWithID<geode::ListenerResult::Propagate>("my-event-id-overridden", test1);
  MyEvent1::globalListenWithID("my-event-id-overridden", test3);
  MyEvent1::globalListenWithIDOn<geode::ListenerResult::Propagate>("my-event-id-overridden", test, &Test::test1);
  MyEvent1::globalListenWithIDOn("my-event-id-overridden", test, &Test::test3);

  MyEvent2::sendWithID<geode::ListenerResult::Propagate>("my-event-id-overridden", test2);
  MyEvent2::sendWithID("my-event-id-overridden", test4);
  MyEvent2::sendWithIDOn<geode::ListenerResult::Propagate>("my-event-id-overridden", test, &Test::test2);
  MyEvent2::sendWithIDOn("my-event-id-overridden", test, &Test::test4);
  MyEvent2::globalSendWithID<geode::ListenerResult::Propagate>("my-event-id-overridden", test2);
  MyEvent2::globalSendWithID("my-event-id-overridden", test4);
  MyEvent2::globalSendWithIDOn<geode::ListenerResult::Propagate>("my-event-id-overridden", test, &Test::test2);
  MyEvent2::globalSendWithIDOn("my-event-id-overridden", test, &Test::test4);

  MyEvent1::rawListenWithID<geode::ListenerResult::Propagate>("my-event-id-overridden", rawTest1);
  MyEvent1::rawListenWithID("my-event-id-overridden", rawTest3);
  MyEvent1::rawListenWithIDOn<geode::ListenerResult::Propagate>("my-event-id-overridden", test, &Test::rawTest1);
  MyEvent1::rawListenWithIDOn("my-event-id-overridden", test, &Test::rawTest3);
  MyEvent1::globalRawListenWithID<geode::ListenerResult::Propagate>("my-event-id-overridden", rawTest1);
  MyEvent1::globalRawListenWithID("my-event-id-overridden", rawTest3);
  MyEvent1::globalRawListenWithIDOn<geode::ListenerResult::Propagate>("my-event-id-overridden", test, &Test::rawTest1);
  MyEvent1::globalRawListenWithIDOn("my-event-id-overridden", test, &Test::rawTest3);

  MyEvent2::rawListenWithID<geode::ListenerResult::Propagate>("my-event-id-overridden", rawTest2);
  MyEvent2::rawListenWithID("my-event-id-overridden", rawTest4);
  MyEvent2::rawListenWithIDOn<geode::ListenerResult::Propagate>("my-event-id-overridden", test, &Test::rawTest2);
  MyEvent2::rawListenWithIDOn("my-event-id-overridden", test, &Test::rawTest4);
  MyEvent2::globalRawListenWithID<geode::ListenerResult::Propagate>("my-event-id-overridden", rawTest2);
  MyEvent2::globalRawListenWithID("my-event-id-overridden", rawTest4);
  MyEvent2::globalRawListenWithIDOn<geode::ListenerResult::Propagate>("my-event-id-overridden", test, &Test::rawTest2);
  MyEvent2::globalRawListenWithIDOn("my-event-id-overridden", test, &Test::rawTest4);

  // "overriding" the id is required if there is no preset id
  MyEvent1NoID::listenWithID<geode::ListenerResult::Propagate>("my-event-id", test1);
  MyEvent1NoID::listenWithID("my-event-id", test3);
  MyEvent1NoID::listenWithIDOn<geode::ListenerResult::Propagate>("my-event-id", test, &Test::test1);
  MyEvent1NoID::listenWithIDOn("my-event-id", test, &Test::test3);
  MyEvent1NoID::globalListenWithID<geode::ListenerResult::Propagate>("my-event-id", test1);
  MyEvent1NoID::globalListenWithID("my-event-id", test3);
  MyEvent1NoID::globalListenWithIDOn<geode::ListenerResult::Propagate>("my-event-id", test, &Test::test1);
  MyEvent1NoID::globalListenWithIDOn("my-event-id", test, &Test::test3);

  MyEvent2NoID::sendWithID<geode::ListenerResult::Propagate>("my-event-id", test2);
  MyEvent2NoID::sendWithID("my-event-id", test4);
  MyEvent2NoID::sendWithIDOn<geode::ListenerResult::Propagate>("my-event-id", test, &Test::test2);
  MyEvent2NoID::sendWithIDOn("my-event-id", test, &Test::test4);
  MyEvent2NoID::globalSendWithID<geode::ListenerResult::Propagate>("my-event-id", test2);
  MyEvent2NoID::globalSendWithID("my-event-id", test4);
  MyEvent2NoID::globalSendWithIDOn<geode::ListenerResult::Propagate>("my-event-id", test, &Test::test2);
  MyEvent2NoID::globalSendWithIDOn("my-event-id", test, &Test::test4);

  MyEvent1NoID::rawListenWithID<geode::ListenerResult::Propagate>("my-event-id", rawTest1);
  MyEvent1NoID::rawListenWithID("my-event-id", rawTest3);
  MyEvent1NoID::rawListenWithIDOn<geode::ListenerResult::Propagate>("my-event-id", test, &Test::rawTest1);
  MyEvent1NoID::rawListenWithIDOn("my-event-id", test, &Test::rawTest3);
  MyEvent1NoID::globalRawListenWithID<geode::ListenerResult::Propagate>("my-event-id", rawTest1);
  MyEvent1NoID::globalRawListenWithID("my-event-id", rawTest3);
  MyEvent1NoID::globalRawListenWithIDOn<geode::ListenerResult::Propagate>("my-event-id", test, &Test::rawTest1);
  MyEvent1NoID::globalRawListenWithIDOn("my-event-id", test, &Test::rawTest3);

  MyEvent2NoID::rawListenWithID<geode::ListenerResult::Propagate>("my-event-id", rawTest2);
  MyEvent2NoID::rawListenWithID("my-event-id", rawTest4);
  MyEvent2NoID::rawListenWithIDOn<geode::ListenerResult::Propagate>("my-event-id", test, &Test::rawTest2);
  MyEvent2NoID::rawListenWithIDOn("my-event-id", test, &Test::rawTest4);
  MyEvent2NoID::globalRawListenWithID<geode::ListenerResult::Propagate>("my-event-id", rawTest2);
  MyEvent2NoID::globalRawListenWithID("my-event-id", rawTest4);
  MyEvent2NoID::globalRawListenWithIDOn<geode::ListenerResult::Propagate>("my-event-id", test, &Test::rawTest2);
  MyEvent2NoID::globalRawListenWithIDOn("my-event-id", test, &Test::rawTest4);
  
  // should not compile, no preset id
  /*
  MyEvent1NoID::listen<geode::ListenerResult::Propagate>(test1);
  MyEvent1NoID::listen(test3);
  MyEvent1NoID::listenOn<geode::ListenerResult::Propagate>(test, &Test::test1);
  MyEvent1NoID::listenOn(test, &Test::test3);
  MyEvent1NoID::globalListen<geode::ListenerResult::Propagate>(test1);
  MyEvent1NoID::globalListen(test3);
  MyEvent1NoID::globalListenOn<geode::ListenerResult::Propagate>(test, &Test::test1);
  MyEvent1NoID::globalListenOn(test, &Test::test3);

  MyEvent2NoID::send<geode::ListenerResult::Propagate>(test2);
  MyEvent2NoID::send(test4);
  MyEvent2NoID::sendOn<geode::ListenerResult::Propagate>(test, &Test::test2);
  MyEvent2NoID::sendOn(test, &Test::test4);
  MyEvent2NoID::globalSend<geode::ListenerResult::Propagate>(test2);
  MyEvent2NoID::globalSend(test4);
  MyEvent2NoID::globalSendOn<geode::ListenerResult::Propagate>(test, &Test::test2);
  MyEvent2NoID::globalSendOn(test, &Test::test4);

  MyEvent1NoID::rawListen<geode::ListenerResult::Propagate>(rawTest1);
  MyEvent1NoID::rawListen(rawTest3);
  MyEvent1NoID::rawListenOn<geode::ListenerResult::Propagate>(test, &Test::rawTest1);
  MyEvent1NoID::rawListenOn(test, &Test::rawTest3);
  MyEvent1NoID::globalRawListen<geode::ListenerResult::Propagate>(rawTest1);
  MyEvent1NoID::globalRawListen(rawTest3);
  MyEvent1NoID::globalRawListenOn<geode::ListenerResult::Propagate>(test, &Test::rawTest1);
  MyEvent1NoID::globalRawListenOn(test, &Test::rawTest3);

  MyEvent2NoID::rawListen<geode::ListenerResult::Propagate>(rawTest2);
  MyEvent2NoID::rawListen(rawTest4);
  MyEvent2NoID::rawListenOn<geode::ListenerResult::Propagate>(test, &Test::rawTest2);
  MyEvent2NoID::rawListenOn(test, &Test::rawTest4);
  MyEvent2NoID::globalRawListen<geode::ListenerResult::Propagate>(rawTest2);
  MyEvent2NoID::globalRawListen(rawTest4);
  MyEvent2NoID::globalRawListenOn<geode::ListenerResult::Propagate>(test, &Test::rawTest2);
  MyEvent2NoID::globalRawListenOn(test, &Test::rawTest4);
  */

}
