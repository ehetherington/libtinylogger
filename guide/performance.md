# Performance

# Measurement of log message CPU time

## Test platforms
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

## Test description.

A simple elapsed time test of each of the available formats was run. For each
format, 1000 log messages were were written to /dev/null. The time/message value
was computed by dividing the total elapsed time by the number of messages.

As other processes could affect this approach, each of these test was repeated
50 times. The minimum, median, mean and maximum times were found.

## Results

All times are in nanoseconds.

### XW6600

 Format             |  min   | median |  mean  |   max   
--------------------|--------|--------|--------|---------
log_fmt_basic       |    284 |    286 |    286 |     311 
log_fmt_systemd     |    336 |    339 |    339 |     344 
log_fmt_standard    |   1371 |   1386 |   1390 |    1565 
log_fmt_debug       |   1609 |   1629 |   1630 |    1678 
log_fmt_debug_tid   |   3617 |   3652 |   3655 |    3837 
log_fmt_debug_tname |   3734 |   3769 |   3772 |    3870 
log_fmt_debug_tall  |   5257 |   5309 |   5312 |    5579 
log_fmt_xml         |   6896 |   6938 |   6938 |    7071 
log_fmt_json        |   9159 |   9229 |   9229 |    9535 


### Raspberry Pi

 Format             |  min   | median |  mean  |   max   
--------------------|--------|--------|--------|---------
log_fmt_basic       |   1004 |   1014 |   1077 |    2727 
log_fmt_systemd     |   1088 |   1098 |   1098 |    1148 
log_fmt_standard    |   2979 |   3012 |   3019 |    3501 
log_fmt_debug       |   3411 |   3433 |   3437 |    4007 
log_fmt_debug_tid   |   3999 |   4032 |   4036 |    4671 
log_fmt_debug_tname |   4169 |   4219 |   4228 |    4897 
log_fmt_debug_tall  |   4764 |   4808 |   4810 |    5547 
log_fmt_xml         |   9762 |   9881 |   9892 |   11406 
log_fmt_json        |  10820 |  11002 |  11038 |   12555 


## Results observations

The median was less than or equal to the mean in all cases, but also very close,
This means that very few large outliers were encountered in these runs.
There were other runs where there were very large maximums, and apparently
several of them, and the mean was noticably larger than the mean. Since the min,
median and mean measurements in these runs were all very close, they seem
reliable.

### XW6600
In the following discussion, the results from the XW6600 are considered.

A timestamp (`clock_gettime()`) is taken for each message, whether or not the
message format includes a timestamp string.

The `basic` and `systemd` formats do no formatting of the timestamp, but still
include the `clock_gettime()` common to all messages. So ~0.3 microseconds is
the baseline time that the other formats add to.

The `standard` format adds formatting of the timestamp with `localtime_r()` and
snprintf(). That seems to account for about 1.1 microseconds.

The `debug_tid` and `debug_tname` formats add either a lookup of thread name
(via `syscall(__NR_gettid`)) or thread id (via `pthread_getname_np`), They seem
to add about 2.1 microsends each. The `tall` format adds both.

The `xml` and `json` structured formats add lots of formatting, for an
additional 2.0 or 4.0 microseconds.

### Raspberry Pi
The RaspberryPi results were slower, as expected, mostly due to the 1.5 MHz
processor. But there is an immediate jump of about 1 microsecond over the
XW6600 results. This prompted measurement of the `clock_gettime()` and
`localtime_r()` functions. Unexpectedly, the `clock_gettime()` function went
from 57 microseconds on the XW660 to 593 microseconds on the RaspberryPi. And
`localtime_r()` function also suffered a disproportional increase.

### Preliminary investigation into time function overhead

#### XW6600

 Function                       | time
--------------------------------|------
clock_gettime()                 | 57
clock_gettime() + localtime_r() | 705
clock_gettime() + localtime_r() | 486

#### Raspberry Pi

 Function                       | time
--------------------------------|------
clock_gettime()                 | 593
clock_gettime() + localtime_r() | 2296
clock_gettime() + localtime_r() | 981

Further investigation will follow.

[guide](./guide.md)
