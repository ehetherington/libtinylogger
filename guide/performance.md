## Performance

### Messages per second logging to file
#### Test platforms
- Raspberry Pi 4b
  - ARMv7 Processor rev 3 (v7l) - Broadcom BCM2711B0 (Cortex A-72) - 1.5 GHz Quad core
  - 4G ram
  - disks
    - mmcblk0   disk 119.3G (SSD Samsung EVO Select 128)
- HP XW6600 workstation
  - Intel(R) Xeon(R) CPU E5430 @ 2.66GHz Quad core
  - 32G ram
  - disks
    - sda   disk  149.1G WDC WD1600AAJS-6
    - sdb   disk  931.5G WDC WD1001FALS-0
    - sdc   disk    1.8T ST2000DM008-2FR1

#### Test description.
Constants:
- N_THREADS
- N_MESSAGES
- MAX_SLEEP

Logging to a file is configured.
N_THREADS threads are created. Each thread will log N_MESSAGES messages.
Between writing messages, the threads sleep a random amount of time from 0 to
MAX_SLEEP microseconds.

After the output file was generated, it was verified that each message
conformed to the output format, and that the correct number of messages
were written.

#### Source:

[beehive.c](demo/beehive.c)

#### Sample file output:

```
2020-06-13 02:13:45.524 INFO      7213:thread_97 sleeping 750 microseconds, s/n=3994, tid=7213
2020-06-13 02:13:45.524 INFO      7233:thread_117 sleeping 33 microseconds, s/n=3899, tid=7233
2020-06-13 02:13:45.524 INFO      7203:thread_87 sleeping 870 microseconds, s/n=3948, tid=7203
2020-06-13 02:13:45.524 INFO      7233:thread_117 sleeping 301 microseconds, s/n=3900, tid=7233
2020-06-13 02:13:45.524 INFO      7169:thread_53 sleeping 838 microseconds, s/n=3930, tid=7169
2020-06-13 02:13:45.524 INFO      7321:thread_205 sleeping 345 microseconds, s/n=3914, tid=7321
2020-06-13 02:13:45.524 INFO      7256:thread_140 sleeping 222 microseconds, s/n=3837, tid=7256
2020-06-13 02:13:45.524 INFO      7236:thread_120 sleeping 561 microseconds, s/n=3948, tid=7236
2020-06-13 02:13:45.525 INFO      7266:thread_150 sleeping 12 microseconds, s/n=3865, tid=7266
2020-06-13 02:13:45.525 INFO      7233:thread_117 sleeping 241 microseconds, s/n=3901, tid=7233
2020-06-13 02:13:45.525 INFO      7120:thread_4 sleeping 378 microseconds, s/n=3968, tid=7120
2020-06-13 02:13:45.525 INFO      7256:thread_140 sleeping 289 microseconds, s/n=3838, tid=7256
2020-06-13 02:13:45.525 INFO      7266:thread_150 sleeping 6 microseconds, s/n=3866, tid=7266
2020-06-13 02:13:45.525 INFO      7352:thread_236 sleeping 37 microseconds, s/n=3982, tid=7352
2020-06-13 02:13:45.525 INFO      7321:thread_205 sleeping 527 microseconds, s/n=3915, tid=7321
2020-06-13 02:13:45.525 INFO      7324:thread_208 sleeping 317 microseconds, s/n=3879, tid=7324
2020-06-13 02:13:45.525 INFO      7266:thread_150 sleeping 47 microseconds, s/n=3867, tid=7266
2020-06-13 02:13:45.525 INFO      7352:thread_236 sleeping 181 microseconds, s/n=3983, tid=7352
2020-06-13 02:13:45.525 INFO      7213:thread_97 sleeping 241 microseconds, s/n=3995, tid=7213
2020-06-13 02:13:45.525 INFO      7233:thread_117 sleeping 152 microseconds, s/n=3902, tid=7233
2020-06-13 02:13:45.525 INFO      7266:thread_150 sleeping 153 microseconds, s/n=3868, tid=7266
2020-06-13 02:13:45.525 INFO      7256:thread_140 sleeping 46 microseconds, s/n=3839, tid=7256
2020-06-13 02:13:45.525 INFO      7120:thread_4 sleeping 629 microseconds, s/n=3969, tid=7120
```

Resulting file info:
```
$ wc /tmp/hive.log
 1000500  9004750 97213984 /tmp/hive.log
```

#### Results - no line buffering

 param         | XW6600 | Raspberry Pi 4b
---------------|--------|----------------
N_THREADS      | 250    | 250
N_MESSAGES     | 4000   | 4000
MAX_SLEEP      | 1000   | 1000
Total Messages | 1,000,000 | 1,000,000
real time      | 0m15.689s | 0m16.733s
user time      | 0m9.551s  | 0m10.681s
sys time       | 0m14.218s | 0m17.121s
messages/second | 63.7 k | 59.9 k
MBytes/sec     | 6.2       | 5.8


#### Results - line buffering (line feed causes write to file)

 param         | XW6600 | Raspberry Pi 4b
---------------|--------|----------------
N_THREADS      | 250    | 250
N_MESSAGES     | 4000   | 4000
MAX_SLEEP      | 1000   | 1000
Total Messages | 1,000,000 | 1,000,000
real time      | 0m23.077s | 0m28.207s
user time      | 0m10.796s | 0m12.486s
sys time       | 0m19.133s | 0m26.316s
messages/second | 43.3 k | 38.0 k
MBytes/sec     | 4.2       | 3.4

#### Notes
- The time to read and check the output was included in the times
listed above.

- No line buffering on the Raspberry Pi was the only case where the sys time
exceeded the real time, meaning it averaged more than 1 core to support the
test.

- 1000 threads was initially used on the xw6600. When it was tried on the
Raspberry Pi, it failed for the following reason:
```
pthread create: Resource temporarily unavailable
```
I reduced the thread count to 500, and it worked the first time, but not
the second. 250 threads works reliably.

- MBytes/sec was an afterthought.

[guide](./guide.md)

