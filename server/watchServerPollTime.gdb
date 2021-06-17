set pagination off
set logging file gdb.output
set logging on


break server.cpp:17622
cont


print anyTicketServerRequestsOut
print areTriggersEnabled()
print pollTimeout
print Time::getCurrentTime()
cont

print anyTicketServerRequestsOut
print areTriggersEnabled()
print pollTimeout
print Time::getCurrentTime()
cont


print anyTicketServerRequestsOut
print areTriggersEnabled()
print pollTimeout
print Time::getCurrentTime()
cont


print anyTicketServerRequestsOut
print areTriggersEnabled()
print pollTimeout
print Time::getCurrentTime()
cont


print anyTicketServerRequestsOut
print areTriggersEnabled()
print pollTimeout
print Time::getCurrentTime()
cont


print anyTicketServerRequestsOut
print areTriggersEnabled()
print pollTimeout
print Time::getCurrentTime()
cont


detach
quit