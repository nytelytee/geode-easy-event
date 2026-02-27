#include <api/easy-event.hpp>

using EasyEvent = nytelyte::easy_event::EasyEvent;

using MyEvent1 = EasyEvent::withID<"my-event-id">::takes<int>;
using MyEvent2 = EasyEvent::withID<"my-event-id">::takes<int>::returns<int>;

using MyEvent1NoID = EasyEvent::takes<int>;
using MyEvent2NoID = EasyEvent::takes<int>::returns<int>;

void test1(int) { return; }
int test2(int x) { return x; }
bool test3(int) { return geode::ListenerResult::Propagate; }
std::pair<bool, int> test4(int x) { return std::pair{geode::ListenerResult::Propagate, x}; }
void rawTest1(std::nullptr_t, int) { return; }
void rawTest2(int*, int) { return; }
bool rawTest3(std::nullptr_t, int) { return geode::ListenerResult::Propagate; }
bool rawTest4(int*, int) { return geode::ListenerResult::Propagate; }

void testSending() {
  geode::ListenerResult result;
  int returnValue;
  
  result = MyEvent1::send(5);
  result = MyEvent1::sendWithID("my-event-id-overridden", 5);
  
  // this should not compile, return value is void, so there is nothing to return
  /*
  returnValue = MyEvent1::sendAndReceive(5);
  returnValue = MyEvent1::sendAndReceiveWithID("my-event-id-overridden", 5);

  std::tie(result, returnValue) = MyEvent1::sendAndReceiveBoth(5);
  std::tie(result, returnValue) = MyEvent1::sendAndReceiveBothWithID("my-event-id-overridden", 5);
  */

  result = MyEvent1::rawSend(nullptr, 5);
  result = MyEvent1::rawSendWithID("my-event-id-overridden", nullptr, 5);
  
  result = MyEvent2::send(5);
  result = MyEvent2::sendWithID("my-event-id-overridden", 5);

  returnValue = MyEvent2::sendAndReceive(5);
  returnValue = MyEvent2::sendAndReceiveWithID("my-event-id-overridden", 5);

  std::tie(result, returnValue) = MyEvent2::sendAndReceiveBoth(5);
  std::tie(result, returnValue) = MyEvent2::sendAndReceiveBothWithID("my-event-id-overridden", 5);

  result = MyEvent2::rawSend(&returnValue, 5);
  result = MyEvent2::rawSendWithID("my-event-id-overridden", &returnValue, 5);

}

void testListening() {

  MyEvent1::listen<geode::ListenerResult::Propagate>(test1).leak();
  MyEvent1::listen(test3).leak();

  MyEvent2::listenAndReturn<geode::ListenerResult::Propagate>(test2).leak();
  MyEvent2::listenAndReturn(test4).leak();

  MyEvent1::rawListen<geode::ListenerResult::Propagate>(rawTest1).leak();
  MyEvent1::rawListen(rawTest3).leak();

  MyEvent2::rawListen<geode::ListenerResult::Propagate>(rawTest2).leak();
  MyEvent2::rawListen(rawTest4).leak();

  // events with preset id allow for overriding it
  MyEvent1::listenWithID<geode::ListenerResult::Propagate>("my-event-id-overridden", test1).leak();
  MyEvent1::listenWithID("my-event-id-overridden", test3).leak();

  MyEvent2::listenAndReturnWithID<geode::ListenerResult::Propagate>("my-event-id-overridden", test2).leak();
  MyEvent2::listenAndReturnWithID("my-event-id-overridden", test4).leak();

  MyEvent1::rawListenWithID<geode::ListenerResult::Propagate>("my-event-id-overridden", rawTest1).leak();
  MyEvent1::rawListenWithID("my-event-id-overridden", rawTest3).leak();

  MyEvent2::rawListenWithID<geode::ListenerResult::Propagate>("my-event-id-overridden", rawTest2).leak();
  MyEvent2::rawListenWithID("my-event-id-overridden", rawTest4).leak();

  // "overriding" the id is required if there is no preset id
  MyEvent1NoID::listenWithID<geode::ListenerResult::Propagate>("my-event-id", test1).leak();
  MyEvent1NoID::listenWithID("my-event-id", test3).leak();

  MyEvent2NoID::listenAndReturnWithID<geode::ListenerResult::Propagate>("my-event-id", test2).leak();
  MyEvent2NoID::listenAndReturnWithID("my-event-id", test4).leak();

  MyEvent1NoID::rawListenWithID<geode::ListenerResult::Propagate>("my-event-id", rawTest1).leak();
  MyEvent1NoID::rawListenWithID("my-event-id", rawTest3).leak();

  MyEvent2NoID::rawListenWithID<geode::ListenerResult::Propagate>("my-event-id", rawTest2).leak();
  MyEvent2NoID::rawListenWithID("my-event-id", rawTest4).leak();
  
  // should not compile, no preset id
  /*
  MyEvent1NoID::listen<geode::ListenerResult::Propagate>(test1).leak();
  MyEvent1NoID::listen(test3).leak();

  MyEvent2NoID::listenAndReturn<geode::ListenerResult::Propagate>(test2).leak();
  MyEvent2NoID::listenAndReturn(test4).leak();

  MyEvent1NoID::rawListen<geode::ListenerResult::Propagate>(rawTest1).leak();
  MyEvent1NoID::rawListen(rawTest3).leak();

  MyEvent2NoID::rawListen<geode::ListenerResult::Propagate>(rawTest2).leak();
  MyEvent2NoID::rawListen(rawTest4).leak();
  */
}
