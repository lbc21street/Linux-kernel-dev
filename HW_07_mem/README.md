# Результаты тестов

## Тест kmalloc

```powershell
[  182.665819] [memtest] MtpSetMemoryTestMode(): got test mode 1
[  182.665968] [memtest] MtInit(): entering...
[  182.665969] [memtest] MtTestMemory(): => total RAM pages: 1686043 (6906032128 bytes)
[  182.665974] [memtest] MtpTestKmalloc(): [kmalloc] 0 - 4096 bytes (1 page(s)) [n/a] spent 0 ms => SUCCESS
[  182.665979] [memtest] MtpTestKmalloc(): [kmalloc] 1 - 8192 bytes (2 page(s)) [contiguous] spent 0 ms => SUCCESS
[  182.665983] [memtest] MtpTestKmalloc(): [kmalloc] 3 - 16384 bytes (4 page(s)) [contiguous] spent 0 ms => SUCCESS
[  182.665989] [memtest] MtpTestKmalloc(): [kmalloc] 7 - 32768 bytes (8 page(s)) [contiguous] spent 0 ms => SUCCESS
[  182.666006] [memtest] MtpTestKmalloc(): [kmalloc] 15 - 65536 bytes (16 page(s)) [contiguous] spent 0 ms => SUCCESS
[  182.666059] [memtest] MtpTestKmalloc(): [kmalloc] 31 - 131072 bytes (32 page(s)) [contiguous] spent 0 ms => SUCCESS
[  182.666263] [memtest] MtpTestKmalloc(): [kmalloc] 63 - 262144 bytes (64 page(s)) [contiguous] spent 0 ms => SUCCESS
[  182.667044] [memtest] MtpTestKmalloc(): [kmalloc] 127 - 524288 bytes (128 page(s)) [contiguous] spent 0 ms => SUCCESS
[  182.670329] [memtest] MtpTestKmalloc(): [kmalloc] 255 - 1048576 bytes (256 page(s)) [contiguous] spent 0 ms => SUCCESS
[  182.687318] [memtest] MtpTestKmalloc(): [kmalloc] 511 - 2097152 bytes (512 page(s)) [contiguous] spent 0 ms => SUCCESS
[  182.759138] [memtest] MtpTestKmalloc(): [kmalloc] 1023 - 4194304 bytes (1024 page(s)) [contiguous] spent 0 ms => SUCCESS
[  182.759164] [memtest] MtpTestKmalloc(): [kmalloc] 1024 - 4198400 bytes (1025 page(s)) spent 0 ms => FAILURE
[  182.759170] [memtest] MtInit(): leaving...
[  187.364497] [memtest] MtExit(): entering...
[  187.364501] [memtest] MtExit(): leaving...
```

## Тест vmalloc

```powershell
[  189.014591] [memtest] MtpSetMemoryTestMode(): got test mode 2
[  189.014739] [memtest] MtInit(): entering...
[  189.014741] [memtest] MtTestMemory(): => total RAM pages: 1686043 (6906032128 bytes)
[  189.014757] [memtest] MtpTestVmalloc(): [vmalloc] 0 - 4096 bytes (1 page(s)) [n/a] spent 0 ms => SUCCESS
[  189.014766] [memtest] MtpTestVmalloc(): [vmalloc] 1 - 8192 bytes (2 page(s)) [non-contiguous] spent 0 ms => SUCCESS
[  189.014773] [memtest] MtpTestVmalloc(): [vmalloc] 2 - 16384 bytes (4 page(s)) [non-contiguous] spent 0 ms => SUCCESS
[  189.014782] [memtest] MtpTestVmalloc(): [vmalloc] 3 - 32768 bytes (8 page(s)) [non-contiguous] spent 0 ms => SUCCESS
[  189.014793] [memtest] MtpTestVmalloc(): [vmalloc] 4 - 65536 bytes (16 page(s)) [non-contiguous] spent 0 ms => SUCCESS
[  189.014807] [memtest] MtpTestVmalloc(): [vmalloc] 5 - 131072 bytes (32 page(s)) [non-contiguous] spent 0 ms => SUCCESS
[  189.014824] [memtest] MtpTestVmalloc(): [vmalloc] 6 - 262144 bytes (64 page(s)) [non-contiguous] spent 0 ms => SUCCESS
[  189.014862] [memtest] MtpTestVmalloc(): [vmalloc] 7 - 524288 bytes (128 page(s)) [non-contiguous] spent 0 ms => SUCCESS
[  189.014943] [memtest] MtpTestVmalloc(): [vmalloc] 8 - 1048576 bytes (256 page(s)) [non-contiguous] spent 0 ms => SUCCESS
[  189.015116] [memtest] MtpTestVmalloc(): [vmalloc] 9 - 2097152 bytes (512 page(s)) [non-contiguous] spent 0 ms => SUCCESS
[  189.015497] [memtest] MtpTestVmalloc(): [vmalloc] 10 - 4194304 bytes (1024 page(s)) [non-contiguous] spent 0 ms => SUCCESS
[  189.016303] [memtest] MtpTestVmalloc(): [vmalloc] 11 - 8388608 bytes (2048 page(s)) [non-contiguous] spent 0 ms => SUCCESS
[  189.018245] [memtest] MtpTestVmalloc(): [vmalloc] 12 - 16777216 bytes (4096 page(s)) [non-contiguous] spent 4 ms => SUCCESS
[  189.022776] [memtest] MtpTestVmalloc(): [vmalloc] 13 - 33554432 bytes (8192 page(s)) [non-contiguous] spent 4 ms => SUCCESS
[  189.039775] [memtest] MtpTestVmalloc(): [vmalloc] 14 - 67108864 bytes (16384 page(s)) [non-contiguous] spent 16 ms => SUCCESS
[  189.076733] [memtest] MtpTestVmalloc(): [vmalloc] 15 - 134217728 bytes (32768 page(s)) [non-contiguous] spent 32 ms => SUCCESS
[  189.147436] [memtest] MtpTestVmalloc(): [vmalloc] 16 - 268435456 bytes (65536 page(s)) [non-contiguous] spent 68 ms => SUCCESS
[  189.284765] [memtest] MtpTestVmalloc(): [vmalloc] 17 - 536870912 bytes (131072 page(s)) [non-contiguous] spent 128 ms => SUCCESS
[  189.555618] [memtest] MtpTestVmalloc(): [vmalloc] 18 - 1073741824 bytes (262144 page(s)) [non-contiguous] spent 256 ms => SUCCESS
[  189.784158] [memtest] MtpTestVmalloc(): [vmalloc] 19 - 1342177280 bytes (327680 page(s)) [non-contiguous] spent 196 ms => SUCCESS
[  190.043576] [memtest] MtpTestVmalloc(): [vmalloc] 20 - 1610612736 bytes (393216 page(s)) [non-contiguous] spent 220 ms => SUCCESS
[  190.332704] [memtest] MtpTestVmalloc(): [vmalloc] 21 - 1879048192 bytes (458752 page(s)) [non-contiguous] spent 240 ms => SUCCESS
[  190.658933] [memtest] MtpTestVmalloc(): [vmalloc] 22 - 2147483648 bytes (524288 page(s)) [non-contiguous] spent 272 ms => SUCCESS
[  191.012413] [memtest] MtpTestVmalloc(): [vmalloc] 23 - 2415919104 bytes (589824 page(s)) [non-contiguous] spent 292 ms => SUCCESS
[  191.395559] [memtest] MtpTestVmalloc(): [vmalloc] 24 - 2684354560 bytes (655360 page(s)) [non-contiguous] spent 316 ms => SUCCESS
[  191.812600] [memtest] MtpTestVmalloc(): [vmalloc] 25 - 2952790016 bytes (720896 page(s)) [non-contiguous] spent 340 ms => SUCCESS
[  192.264676] [memtest] MtpTestVmalloc(): [vmalloc] 26 - 3221225472 bytes (786432 page(s)) [non-contiguous] spent 364 ms => SUCCESS
[  192.738735] [memtest] MtpTestVmalloc(): [vmalloc] 27 - 3489660928 bytes (851968 page(s)) [non-contiguous] spent 384 ms => SUCCESS
[  193.244506] [memtest] MtpTestVmalloc(): [vmalloc] 28 - 3758096384 bytes (917504 page(s)) [non-contiguous] spent 408 ms => SUCCESS
[  193.794265] [memtest] MtpTestVmalloc(): [vmalloc] 29 - 4026531840 bytes (983040 page(s)) [non-contiguous] spent 444 ms => SUCCESS
[  194.359773] [memtest] MtpTestVmalloc(): [vmalloc] 30 - 4294967296 bytes (1048576 page(s)) [non-contiguous] spent 452 ms => SUCCESS
[  194.954531] [memtest] MtpTestVmalloc(): [vmalloc] 31 - 4563402752 bytes (1114112 page(s)) [non-contiguous] spent 476 ms => SUCCESS
[  195.579948] [memtest] MtpTestVmalloc(): [vmalloc] 32 - 4831838208 bytes (1179648 page(s)) [non-contiguous] spent 496 ms => SUCCESS
[  196.234532] [memtest] MtpTestVmalloc(): [vmalloc] 33 - 5100273664 bytes (1245184 page(s)) [non-contiguous] spent 520 ms => SUCCESS
[  196.919737] [memtest] MtpTestVmalloc(): [vmalloc] 34 - 5368709120 bytes (1310720 page(s)) [non-contiguous] spent 544 ms => SUCCESS
[  197.634462] [memtest] MtpTestVmalloc(): [vmalloc] 35 - 5637144576 bytes (1376256 page(s)) [non-contiguous] spent 568 ms => SUCCESS
[  198.522881] [memtest] MtpTestVmalloc(): [vmalloc] 36 - 5905580032 bytes (1441792 page(s)) spent 732 ms => FAILURE
[  198.522892] [memtest] MtInit(): leaving...
[  200.172807] [memtest] MtExit(): entering...
[  200.172816] [memtest] MtExit(): leaving...
```

## Тест kmem_cache

```powershell
[  202.815788] [memtest] MtpSetMemoryTestMode(): got test mode 3
[  202.816135] [memtest] MtInit(): entering...
[  202.816138] [memtest] MtTestMemory(): => total RAM pages: 1686043 (6906032128 bytes)
[  202.816300] [memtest] MtpTestKmemCache(): created kmem_cache <TestKmemCache> 0xffff888123497a40 - blockSize 4096
[  202.816313] [memtest] MtpTestKmemCache(): [kmem_cache] 0 - 4096 bytes (1 page(s)) [n/a] spent 0 ms => SUCCESS
[  202.816320] [memtest] MtpTestKmemCache(): [kmem_cache] 1 - 8192 bytes (2 page(s)) [non-contiguous] spent 0 ms => SUCCESS
[  202.816327] [memtest] MtpTestKmemCache(): [kmem_cache] 3 - 16384 bytes (4 page(s)) [non-contiguous] spent 0 ms => SUCCESS
[  202.816355] [memtest] MtpTestKmemCache(): [kmem_cache] 7 - 32768 bytes (8 page(s)) [non-contiguous] spent 0 ms => SUCCESS
[  202.816376] [memtest] MtpTestKmemCache(): [kmem_cache] 15 - 65536 bytes (16 page(s)) [non-contiguous] spent 0 ms => SUCCESS
[  202.816416] [memtest] MtpTestKmemCache(): [kmem_cache] 31 - 131072 bytes (32 page(s)) [non-contiguous] spent 0 ms => SUCCESS
[  202.816495] [memtest] MtpTestKmemCache(): [kmem_cache] 63 - 262144 bytes (64 page(s)) [non-contiguous] spent 0 ms => SUCCESS
[  202.816647] [memtest] MtpTestKmemCache(): [kmem_cache] 127 - 524288 bytes (128 page(s)) [non-contiguous] spent 0 ms => SUCCESS
[  202.816939] [memtest] MtpTestKmemCache(): [kmem_cache] 255 - 1048576 bytes (256 page(s)) [non-contiguous] spent 0 ms => SUCCESS
[  202.817739] [memtest] MtpTestKmemCache(): [kmem_cache] 511 - 2097152 bytes (512 page(s)) [non-contiguous] spent 0 ms => SUCCESS
[  202.818739] [memtest] MtpTestKmemCache(): [kmem_cache] 1023 - 4194304 bytes (1024 page(s)) [non-contiguous] spent 0 ms => SUCCESS
[  202.820417] [memtest] MtpTestKmemCache(): [kmem_cache] 2047 - 8388608 bytes (2048 page(s)) [non-contiguous] spent 0 ms => SUCCESS
[  202.822997] [memtest] MtpTestKmemCache(): [kmem_cache] 4095 - 16777216 bytes (4096 page(s)) [non-contiguous] spent 0 ms => SUCCESS
[  202.827721] [memtest] MtpTestKmemCache(): [kmem_cache] 8191 - 33554432 bytes (8192 page(s)) [non-contiguous] spent 0 ms => SUCCESS
[  202.837265] [memtest] MtpTestKmemCache(): [kmem_cache] 16383 - 67108864 bytes (16384 page(s)) [non-contiguous] spent 0 ms => SUCCESS
[  202.856728] [memtest] MtpTestKmemCache(): [kmem_cache] 32767 - 134217728 bytes (32768 page(s)) [non-contiguous] spent 0 ms => SUCCESS
[  202.895229] [memtest] MtpTestKmemCache(): [kmem_cache] 65535 - 268435456 bytes (65536 page(s)) [non-contiguous] spent 0 ms => SUCCESS
[  202.971383] [memtest] MtpTestKmemCache(): [kmem_cache] 131071 - 536870912 bytes (131072 page(s)) [non-contiguous] spent 0 ms => SUCCESS
[  203.121353] [memtest] MtpTestKmemCache(): [kmem_cache] 262143 - 1073741824 bytes (262144 page(s)) [non-contiguous] spent 0 ms => SUCCESS
[  203.419260] [memtest] MtpTestKmemCache(): [kmem_cache] 524287 - 2147483648 bytes (524288 page(s)) [non-contiguous] spent 0 ms => SUCCESS
[  204.014447] [memtest] MtpTestKmemCache(): [kmem_cache] 1048575 - 4294967296 bytes (1048576 page(s)) [non-contiguous] spent 0 ms => SUCCESS
[  204.257629] [memtest] MtpTestKmemCache(): [kmem_cache] 1256497 - 5146615808 bytes (1256498 page(s)) spent 0 ms => FAILURE
[  205.287149] [memtest] MtpTestKmemCache(): destroying kmem_cache <TestKmemCache> 0xffff888123497a40
[  205.642348] [memtest] MtInit(): leaving...
[  208.703096] [memtest] MtExit(): entering...
[  208.703100] [memtest] MtExit(): leaving...
```

## Тест mempool

Небольшое пояснение. В отличие от тестов **vmalloc** и **kmem_cache**, в данном тесте флаг **\_\_GFP_NORETRY** не сработал. Пришлось ограничить количество выделяемой памяти в данном тесте. На ядре **6.1.130** это четверть от **\_totalrampages**, на ядре **6.8.0-40-generic** система нормально переживает три четверти от **\_totalrampages**.

```powershell
[  210.666395] [memtest] MtpSetMemoryTestMode(): got test mode 4
[  210.666558] [memtest] MtInit(): entering...
[  210.666560] [memtest] MtTestMemory(): => total RAM pages: 1686043 (6906032128 bytes)
[  210.668703] [memtest] MtpTestMempool(): created mempool 0xffff8881010bd500 - minBlocks 1024 blockSize 4096
[  210.668708] [memtest] MtpTestMempool(): allocation limit: 1726508032 bytes
[  210.668716] [memtest] MtpTestMempool(): [mempool] 0 - 4096 bytes (1 page(s)) [n/a] spent 0 ms => SUCCESS
[  210.668719] [memtest] MtpTestMempool(): [mempool] 1 - 8192 bytes (2 page(s)) [non-contiguous] spent 0 ms => SUCCESS
[  210.668732] [memtest] MtpTestMempool(): [mempool] 3 - 16384 bytes (4 page(s)) [non-contiguous] spent 0 ms => SUCCESS
[  210.668740] [memtest] MtpTestMempool(): [mempool] 7 - 32768 bytes (8 page(s)) [non-contiguous] spent 0 ms => SUCCESS
[  210.668753] [memtest] MtpTestMempool(): [mempool] 15 - 65536 bytes (16 page(s)) [non-contiguous] spent 0 ms => SUCCESS
[  210.668780] [memtest] MtpTestMempool(): [mempool] 31 - 131072 bytes (32 page(s)) [non-contiguous] spent 0 ms => SUCCESS
[  210.668837] [memtest] MtpTestMempool(): [mempool] 63 - 262144 bytes (64 page(s)) [non-contiguous] spent 0 ms => SUCCESS
[  210.668960] [memtest] MtpTestMempool(): [mempool] 127 - 524288 bytes (128 page(s)) [non-contiguous] spent 0 ms => SUCCESS
[  210.669169] [memtest] MtpTestMempool(): [mempool] 255 - 1048576 bytes (256 page(s)) [non-contiguous] spent 0 ms => SUCCESS
[  210.669610] [memtest] MtpTestMempool(): [mempool] 511 - 2097152 bytes (512 page(s)) [non-contiguous] spent 0 ms => SUCCESS
[  210.670448] [memtest] MtpTestMempool(): [mempool] 1023 - 4194304 bytes (1024 page(s)) [non-contiguous] spent 0 ms => SUCCESS
[  210.672129] [memtest] MtpTestMempool(): [mempool] 2047 - 8388608 bytes (2048 page(s)) [non-contiguous] spent 0 ms => SUCCESS
[  210.675411] [memtest] MtpTestMempool(): [mempool] 4095 - 16777216 bytes (4096 page(s)) [non-contiguous] spent 0 ms => SUCCESS
[  210.681949] [memtest] MtpTestMempool(): [mempool] 8191 - 33554432 bytes (8192 page(s)) [non-contiguous] spent 0 ms => SUCCESS
[  210.694824] [memtest] MtpTestMempool(): [mempool] 16383 - 67108864 bytes (16384 page(s)) [non-contiguous] spent 0 ms => SUCCESS
[  210.722464] [memtest] MtpTestMempool(): [mempool] 32767 - 134217728 bytes (32768 page(s)) [non-contiguous] spent 0 ms => SUCCESS
[  210.774063] [memtest] MtpTestMempool(): [mempool] 65535 - 268435456 bytes (65536 page(s)) [non-contiguous] spent 0 ms => SUCCESS
[  210.873261] [memtest] MtpTestMempool(): [mempool] 131071 - 536870912 bytes (131072 page(s)) [non-contiguous] spent 0 ms => SUCCESS
[  211.069511] [memtest] MtpTestMempool(): [mempool] 262143 - 1073741824 bytes (262144 page(s)) [non-contiguous] spent 0 ms => SUCCESS
[  211.669293] [memtest] MtpTestMempool(): destroying mempool 0xffff8881010bd500
[  211.670294] [memtest] MtInit(): leaving...
[  215.834186] [memtest] MtExit(): entering...
[  215.834194] [memtest] MtExit(): leaving...
```

## Тест \_\_get_free_pages

```powershell
[  218.251881] [memtest] MtpSetMemoryTestMode(): got test mode 5
[  218.252211] [memtest] MtInit(): entering...
[  218.252215] [memtest] MtTestMemory(): => total RAM pages: 1686043 (6906032128 bytes)
[  218.252220] [memtest] MtpTestGetFreePages(): [__get_free_pages] 0 - 4096 bytes (1 page(s)) [n/a] spent 0 ms => SUCCESS
[  218.252227] [memtest] MtpTestGetFreePages(): [__get_free_pages] 1 - 8192 bytes (2 page(s)) [contiguous] spent 0 ms => SUCCESS
[  218.252234] [memtest] MtpTestGetFreePages(): [__get_free_pages] 2 - 16384 bytes (4 page(s)) [contiguous] spent 0 ms => SUCCESS
[  218.252241] [memtest] MtpTestGetFreePages(): [__get_free_pages] 3 - 32768 bytes (8 page(s)) [contiguous] spent 0 ms => SUCCESS
[  218.252253] [memtest] MtpTestGetFreePages(): [__get_free_pages] 4 - 65536 bytes (16 page(s)) [contiguous] spent 0 ms => SUCCESS
[  218.252268] [memtest] MtpTestGetFreePages(): [__get_free_pages] 5 - 131072 bytes (32 page(s)) [contiguous] spent 0 ms => SUCCESS
[  218.252290] [memtest] MtpTestGetFreePages(): [__get_free_pages] 6 - 262144 bytes (64 page(s)) [contiguous] spent 0 ms => SUCCESS
[  218.252349] [memtest] MtpTestGetFreePages(): [__get_free_pages] 7 - 524288 bytes (128 page(s)) [contiguous] spent 0 ms => SUCCESS
[  218.252429] [memtest] MtpTestGetFreePages(): [__get_free_pages] 8 - 1048576 bytes (256 page(s)) [contiguous] spent 0 ms => SUCCESS
[  218.252682] [memtest] MtpTestGetFreePages(): [__get_free_pages] 9 - 2097152 bytes (512 page(s)) [contiguous] spent 0 ms => SUCCESS
[  218.253184] [memtest] MtpTestGetFreePages(): [__get_free_pages] 10 - 4194304 bytes (1024 page(s)) [contiguous] spent 4 ms => SUCCESS
[  218.253233] [memtest] MtpTestGetFreePages(): [__get_free_pages] 11 - 8388608 bytes (2048 page(s)) spent 0 ms => FAILURE
[  218.253253] [memtest] MtInit(): leaving...
[  219.734026] [memtest] MtExit(): entering...
[  219.734030] [memtest] MtExit(): leaving...
```

## Тест alloc_pages

```powershell
[  221.941760] [memtest] MtpSetMemoryTestMode(): got test mode 6
[  221.941923] [memtest] MtInit(): entering...
[  221.941924] [memtest] MtTestMemory(): => total RAM pages: 1686043 (6906032128 bytes)
[  221.941926] [memtest] MtpTestAllocPages(): [alloc_pages] 0 - 4096 bytes (1 page(s)) [n/a] spent 0 ms => SUCCESS
[  221.941929] [memtest] MtpTestAllocPages(): [alloc_pages] 1 - 8192 bytes (2 page(s)) [contiguous] spent 0 ms => SUCCESS
[  221.941933] [memtest] MtpTestAllocPages(): [alloc_pages] 2 - 16384 bytes (4 page(s)) [contiguous] spent 0 ms => SUCCESS
[  221.941937] [memtest] MtpTestAllocPages(): [alloc_pages] 3 - 32768 bytes (8 page(s)) [contiguous] spent 0 ms => SUCCESS
[  221.941943] [memtest] MtpTestAllocPages(): [alloc_pages] 4 - 65536 bytes (16 page(s)) [contiguous] spent 0 ms => SUCCESS
[  221.941955] [memtest] MtpTestAllocPages(): [alloc_pages] 5 - 131072 bytes (32 page(s)) [contiguous] spent 0 ms => SUCCESS
[  221.941969] [memtest] MtpTestAllocPages(): [alloc_pages] 6 - 262144 bytes (64 page(s)) [contiguous] spent 0 ms => SUCCESS
[  221.941994] [memtest] MtpTestAllocPages(): [alloc_pages] 7 - 524288 bytes (128 page(s)) [contiguous] spent 0 ms => SUCCESS
[  221.942073] [memtest] MtpTestAllocPages(): [alloc_pages] 8 - 1048576 bytes (256 page(s)) [contiguous] spent 0 ms => SUCCESS
[  221.942247] [memtest] MtpTestAllocPages(): [alloc_pages] 9 - 2097152 bytes (512 page(s)) [contiguous] spent 0 ms => SUCCESS
[  221.942619] [memtest] MtpTestAllocPages(): [alloc_pages] 10 - 4194304 bytes (1024 page(s)) [contiguous] spent 0 ms => SUCCESS
[  221.942652] [memtest] MtpTestAllocPages(): [alloc_pages] 11 - 8388608 bytes (2048 page(s)) spent 0 ms => FAILURE
[  221.942657] [memtest] MtInit(): leaving...
[  223.860513] [memtest] MtExit(): entering...
[  223.860533] [memtest] MtExit(): leaving...
```
