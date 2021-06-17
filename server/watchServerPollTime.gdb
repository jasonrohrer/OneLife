set pagination off
set logging file gdb.output
set logging on


break 17622
cont


print pollTimeout
print Time::getCurrentTime()
cont

print pollTimeout
print Time::getCurrentTime()
cont


print pollTimeout
print Time::getCurrentTime()
cont


print pollTimeout
print Time::getCurrentTime()
cont


print pollTimeout
print Time::getCurrentTime()
cont


print pollTimeout
print Time::getCurrentTime()
cont


detach
quit