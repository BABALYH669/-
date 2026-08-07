---
notion-id: 3855d378-8ef5-80cd-b6ee-f695bf026f03
---
同等优先级任务轮流地享有相同的 CPU 时间(可设置)， 叫时间片，在FreeRTOS中，一个时间片就等于SysTick 中断周期


![[wps_jno2wdpbxB.png]]

![[wps_CKc7h9NLKD.png]]

串口打印printf也会耗时,所以50ms的时间 delay_ms(10)占了10ms,打印也占了时间,会打印4~5次,加上临界区是为了让任务执行完再切换

## `configTICK_RATE_HZ` 的含义

`configTICK_RATE_HZ` 是**每秒的 tick 次数**，单位是 Hz（赫兹），也就是"次/秒"。

它与每个 tick 的时间间隔是**倒数关系**：

```plain text
每个 tick 的时间 = 1 / configTICK_RATE_HZ
```

## 50ms 对应 20 的推导

```plain text
50ms = 0.05 秒

configTICK_RATE_HZ = 1 / 0.05 = 20 (Hz)
```

也就是说：**1 秒钟产生 20 次 tick，每次间隔正好 50ms**。

## 常见对照表

| 期望间隔 | configTICK_RATE_HZ | 说明 |
| --- | --- | --- |
| 1 ms | 1000 | 默认常用值，精度高但中断开销大 |
| 2 ms | 500 |   |
| 5 ms | 200 |   |
| 10 ms | 100 |   |
| 20 ms | 50 |   |
| **50 ms** | **20** | 你的场景，低频低功耗 |
| 100 ms | 10 | 极低功耗场景 |

## 50ms tick 的影响

把频率降到 20Hz 意味着：

1. `**vTaskDelay(1)**`** 就是 50ms**——所有延时都以 50ms 为最小粒度，`vTaskDelay(2)` = 100ms，依次类推。
2. `**pdMS_TO_TICKS(ms)**`** 宏会按此换算**：
```c
pdMS_TO_TICKS(30) → 30 / 50 = 0 个 tick   // 注意：30ms 会被截断为 0！
pdMS_TO_TICKS(50) → 50 / 50 = 1 个 tick
pdMS_TO_TICKS(80) → 80 / 50 = 1 个 tick   // 也是 1，精度损失
```
3. **CPU 中断开销大幅降低**——从每秒 1000 次中断降到 20 次，非常适合对实时性要求不高但看重功耗的场景。
4. **时间片轮转粒度也变成 50ms**——同优先级任务切换的粒度变粗。

**总结**：`20 Hz` 就是数学上 `1/0.05s` 的结果，不是随意取的。如果你需要更细的时间粒度（比如要能延时 10ms），就得把频率调高，比如 100Hz（10ms 一个 tick）。这是一个精度与开销之间的权衡。