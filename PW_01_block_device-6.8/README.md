# Проектная работа 'Реализация блочного устройства с дисками в памяти'

## Условия проектной работы

- Реализовать драйвер блочного устройства с функциями **read**, **write**, **ioctl**.
- Зарегистрировать драйвер в файловых системах **/proc**, **/sys**, **/dev**.
- На устройстве должно быть не менее трех разделов
- Каждый раздел должен быть размером **100 MiB**

Для хранения дисковых данных блочные устройства используют независимые, заранее аллоцированные буферы в виртуальной памяти. В драйвере были реализованы два механизма обработки запросов (переключаются при сборке дефайном): **submit_bio** и **blk-mq** (последний с асинхронной обработкой в **workqueue**). В случае использования **blk-mq** каждое блочное устройство имеет свою **workqueue**.

## Цели работы

Продемонстрировать навыки владения структурами ядра, работу с асинхронными механизмами в различных контекстах, использование примитивов синхронизации, работу с файловой системой **sysfs**, работу с механизмами блочного **I/O** **submit_bio** и **blk-mq** и взаимодействие с блочной подсистемой ядра.

## Сборка модуля

Для сборки модуля используются стандартные файлы **Kbuild** и **Makefile**. Дополнительно для удобства были созданы файлы **code-workspace** и **tasks.json** для среды разработки **VSCode** (вместе с сопутствующими файлами с различными настройками проекта).

## Архитектура модуля

Проект состоит из следующих модулей (компонентов):
- **devicesupport.c** - все, что связано с созданием и удалением блочных устройств, а также с выделением и освобождением памяти для хранения дисковых данных
- **init.c** - код инициализации и деинициализации драйвера
- **iosupport.c** - обработка отдельных I/O запросов (**bio_vec**) из сегментов, обработка стандартных IOCTL, обработка (для диагностики) некоторых вызовов из **block_device_operations**
- **paramsupport.c** - обработка параметров, передаваемых при загрузке драйвера
- **queuesupport.c** - обработка основных запросов из **blk_mq_ops**, а также некоторых других вызовов для диагностики
- **sysfssupport.c** - все, что связано с созданием и удалением класса, устройств класса, атрибутов класса и атрибутов устройств
- **workqueuesupport.c** - все, что свзяано с отправкой и обработкой запросов в **workqueue**

В проекте используются некоторые упрощения, например, все запросы из **blk-mq** отправляются в **workqueue**, то есть, обрабатываются асинхронно, хотя в запросах есть соответствующий флаг, показывающий синхронность запросов. В варианте **submit_bio** все запросы выполняются синхронно, так как в процессе отладки ни разу не была замечена ситуация контекста **atomic**. Также отсутствует какой-либо учет приоритетов запросов и оптимизация использования **workitem**'ов.

Кроме этого, в драйвере поддержан вариант сборки с пересчетом размера диска после расчета геометрии в терминах устаревшей схемы **CHS** (которой, тем не менее, пользуется утилита **fdisk**). Но так как все соврменные ОС ориентируются на максимальные значения LBA блочных устройств, то данная корректировка размера диска является опциональной.

## Параметры драйвера и интерфейс управления (sysfs)

Для установки параметров всех созданных блочных устройств драйвера, а также динамической настройки трейса в драйвере используются следующие параметры **sysfs**:

```bash
# ls -l /sys/module/pwblkdev/parameters
total 0
-rw-r--r-- 1 root root 4096 ноя  3 18:49 devicecount
-rw-r--r-- 1 root root 4096 ноя  3 18:49 disksize
-rw-r--r-- 1 root root 4096 ноя  3 18:49 partitioncount
-rw-r--r-- 1 root root 4096 ноя  3 18:49 sectorsize
-rw-r--r-- 1 root root 4096 ноя  3 18:49 tracelevel
-rw-r--r-- 1 root root 4096 ноя  3 18:49 tracemask
```

Следующие параметры являются "одноразовыми" (считываются только при загрузке модуля):
- **devicecount** - максимальное количество устройств, которые может создать драйвер
- **disksize** - размер диска в МБ (к нему прибавляется еще 4 МБ для накладных расходов)
- **partitioncount** - максимальное количество разделов на диске (см. замечание ниже)
- **sectorsize** - физический размер сектора диска (см. замечание ниже)

Эти параметры используются для установки уровня трассировки и маски компонентов, которые выводят какой-либо трейс:
- **tracelevel** - уровень трассировки (печатается только трейс, у которого уровень меньше или равен данному уровню трассировки, значение **0** отключает любой трейс), значение по умолчанию **1**
- **tracemask** - 32-битовая маска компонентов, которые выводят трейс, по умолчанию включен трейс от всех компонентов драйвера (**0x0000007F**)

Доступные маски компонентов:

```c
#define PWBD_TM_INIT                0x00000001
#define PWBD_TM_DEVICE_SUPPORT      0x00000002
#define PWBD_TM_IO_SUPPORT          0x00000004
#define PWBD_TM_PARAM_SUPPORT       0x00000008
#define PWBD_TM_QUEUE_SUPPORT       0x00000010
#define PWBD_TM_SYSFS_SUPPORT       0x00000020
#define PWBD_TM_WORKQUEUE_SUPPORT   0x00000040
```

Пример загрузки модуля с указанием параметров:

```bash
# sudo insmod pwblkdev.ko devicecount=5 partitioncount=3 disksize=300 tracelevel=2
```

У класса "виртуальных" устройств данного драйвера есть атирибут класса **add**, который позволяет добавлять новые блочные устройства (если текущее количество устройств меньше установленного максимума):

```bash
# ls -l /sys/class/pwbd/
total 0
-rw-r--r-- 1 root root 4096 окт 28 21:04 add
lrwxrwxrwx 1 root root    0 окт 28 21:04 pwbd0 -> ../../devices/virtual/pwbd/pwbd0
lrwxrwxrwx 1 root root    0 окт 28 21:04 pwbd1 -> ../../devices/virtual/pwbd/pwbd1
lrwxrwxrwx 1 root root    0 окт 28 21:04 pwbd2 -> ../../devices/virtual/pwbd/pwbd2
lrwxrwxrwx 1 root root    0 окт 28 21:04 pwbd3 -> ../../devices/virtual/pwbd/pwbd3
lrwxrwxrwx 1 root root    0 окт 28 21:04 pwbd4 -> ../../devices/virtual/pwbd/pwbd4
```

Для этого достаточно записать в этот атрибут **1**.

И у каждого "виртуального" устройства драйвера есть атрибут устройства **remove**, который позволяет удалить связанное с ним одноименное блочное устройство:

```bash
# ls -l /sys/class/pwbd/pwbd0/
total 0
drwxr-xr-x 2 root root    0 окт 28 21:12 power
-rw-r--r-- 1 root root 4096 окт 28 21:12 remove
lrwxrwxrwx 1 root root    0 окт 28 21:12 subsystem -> ../../../../class/pwbd
-rw-r--r-- 1 root root 4096 окт 28 21:12 uevent
```

Для этого также достаточно записать в этот атрибут **1**.

## Пример использования

Созданим на диске таблицу разделов **GPT** и три раздела с типом по умолчанию (**Linux filesystem**):

```bash
# sudo fdisk /dev/pwbd0

Welcome to fdisk (util-linux 2.39.3).
Changes will remain in memory only, until you decide to write them.
Be careful before using the write command.


Command (m for help): g
Created a new GPT disklabel (GUID: DAFD3B93-2993-4B25-94C6-6A665FD02D81).

Command (m for help): n
Partition number (1-128, default 1): 
First sector (2048-622558, default 2048): 
Last sector, +/-sectors or +/-size{K,M,G,T,P} (2048-622558, default 620543): +100M

Created a new partition 1 of type 'Linux filesystem' and of size 100 MiB.

Command (m for help): n
Partition number (2-128, default 2): 
First sector (206848-622558, default 206848): 
Last sector, +/-sectors or +/-size{K,M,G,T,P} (206848-622558, default 620543): +100M

Created a new partition 2 of type 'Linux filesystem' and of size 100 MiB.

Command (m for help): n
Partition number (3-128, default 3): 
First sector (411648-622558, default 411648): 
Last sector, +/-sectors or +/-size{K,M,G,T,P} (411648-622558, default 620543): +100M

Created a new partition 3 of type 'Linux filesystem' and of size 100 MiB.

Command (m for help): p
Disk /dev/pwbd0: 304 MiB, 318767104 bytes, 622592 sectors
Units: sectors of 1 * 512 = 512 bytes
Sector size (logical/physical): 512 bytes / 512 bytes
I/O size (minimum/optimal): 512 bytes / 512 bytes
Disklabel type: gpt
Disk identifier: DAFD3B93-2993-4B25-94C6-6A665FD02D81

Device        Start    End Sectors  Size Type
/dev/pwbd0p1   2048 206847  204800  100M Linux filesystem
/dev/pwbd0p2 206848 411647  204800  100M Linux filesystem
/dev/pwbd0p3 411648 616447  204800  100M Linux filesystem

Command (m for help): F
Unpartitioned space /dev/pwbd0: 2,98 MiB, 3128832 bytes, 6111 sectors
Units: sectors of 1 * 512 = 512 bytes
Sector size (logical/physical): 512 bytes / 512 bytes

 Start    End Sectors Size
616448 622558    6111   3M

Command (m for help): w
The partition table has been altered.
Calling ioctl() to re-read partition table.
Syncing disks.
```

Отформатируем разделы и посмотрим информацию о диске и созаднных на нем разделах:

```bash
# lsblk /dev/pwbd0
NAME      MAJ:MIN RM  SIZE RO TYPE MOUNTPOINTS
pwbd0     251:0    0  304M  0 disk 
├─pwbd0p1 251:1    0  100M  0 part 
├─pwbd0p2 251:2    0  100M  0 part 
└─pwbd0p3 251:3    0  100M  0 part 

# sudo fdisk -l /dev/pwbd0
Disk /dev/pwbd0: 304 MiB, 318767104 bytes, 622592 sectors
Units: sectors of 1 * 512 = 512 bytes
Sector size (logical/physical): 512 bytes / 512 bytes
I/O size (minimum/optimal): 512 bytes / 512 bytes
Disklabel type: gpt
Disk identifier: DAFD3B93-2993-4B25-94C6-6A665FD02D81

Device        Start    End Sectors  Size Type
/dev/pwbd0p1   2048 206847  204800  100M Linux filesystem
/dev/pwbd0p2 206848 411647  204800  100M Linux filesystem
/dev/pwbd0p3 411648 616447  204800  100M Linux filesystem

# sudo blkid -p /dev/pwbd0p1
/dev/pwbd0p1: SEC_TYPE="msdos" LABEL_FATBOOT="EFI_PART" LABEL="EFI_PART" UUID="A8F6-5745" VERSION="FAT16" FSBLOCKSIZE="2048" BLOCK_SIZE="512" FSSIZE="104832000" TYPE="vfat" USAGE="filesystem" PART_ENTRY_SCHEME="gpt" PART_ENTRY_UUID="1c3c366f-a472-4d75-a67c-b9a6a5251ae6" PART_ENTRY_TYPE="0fc63daf-8483-4772-8e79-3d69d8477de4" PART_ENTRY_NUMBER="1" PART_ENTRY_OFFSET="2048" PART_ENTRY_SIZE="204800" PART_ENTRY_DISK="251:0"

# sudo blkid -p /dev/pwbd0p2
/dev/pwbd0p2: LABEL="Ext4_Data1" UUID="ea814988-3255-410c-9d82-e16ba7447769" VERSION="1.0" FSBLOCKSIZE="4096" BLOCK_SIZE="4096" FSLASTBLOCK="25600" FSSIZE="104857600" TYPE="ext4" USAGE="filesystem" PART_ENTRY_SCHEME="gpt" PART_ENTRY_UUID="2698a244-6e9b-4722-b26b-0ef895567831" PART_ENTRY_TYPE="0fc63daf-8483-4772-8e79-3d69d8477de4" PART_ENTRY_NUMBER="2" PART_ENTRY_OFFSET="206848" PART_ENTRY_SIZE="204800" PART_ENTRY_DISK="251:0"

# sudo blkid -p /dev/pwbd0p3
/dev/pwbd0p3: LABEL="Ext4_Data2" UUID="f38d6d74-8e6d-4537-ae61-6f508373d709" VERSION="1.0" FSBLOCKSIZE="4096" BLOCK_SIZE="4096" FSLASTBLOCK="25600" FSSIZE="104857600" TYPE="ext4" USAGE="filesystem" PART_ENTRY_SCHEME="gpt" PART_ENTRY_UUID="17cdb04b-8935-4fea-838c-5c7a9a87d53f" PART_ENTRY_TYPE="0fc63daf-8483-4772-8e79-3d69d8477de4" PART_ENTRY_NUMBER="3" PART_ENTRY_OFFSET="411648" PART_ENTRY_SIZE="204800" PART_ENTRY_DISK="251:0"
```

Смонтируем созданные разделы:

```bash
# lsblk /dev/pwbd0
NAME      MAJ:MIN RM  SIZE RO TYPE MOUNTPOINTS
pwbd0     251:0    0  304M  0 disk 
├─pwbd0p1 251:1    0  100M  0 part /home/test/pwbd/1
├─pwbd0p2 251:2    0  100M  0 part /home/test/pwbd/2
└─pwbd0p3 251:3    0  100M  0 part /home/test/pwbd/3

# lsblk -p -f /dev/pwbd0
NAME           FSTYPE FSVER LABEL UUID FSAVAIL FSUSE% MOUNTPOINTS
/dev/pwbd0                                            
├─/dev/pwbd0p1                           99,8M     0% /home/test/pwbd/1
├─/dev/pwbd0p2                           82,7M     0% /home/test/pwbd/2
└─/dev/pwbd0p3                           82,7M     0% /home/test/pwbd/3
```

Скопируем какие-нибудь файлы на каждый раздел, размонтируем разделы и выполним проверку:

```bash
# sudo fsck -r /dev/pwbd0p1
fsck from util-linux 2.39.3
fsck.fat 4.2 (2021-01-31)
/dev/pwbd0p1: 18 files, 1232/51078 clusters
/dev/pwbd0p1: status 0, rss 2816, real 0.004375, user 0.001389, sys 0.002778

# sudo fsck -r /dev/pwbd0p2
fsck from util-linux 2.39.3
e2fsck 1.47.0 (5-Feb-2023)
Ext4_Data1: clean, 28/25600 files, 3268/25600 blocks
/dev/pwbd0p2: status 0, rss 3200, real 0.004175, user 0.002258, sys 0.001334

# sudo fsck -r /dev/pwbd0p3
fsck from util-linux 2.39.3
e2fsck 1.47.0 (5-Feb-2023)
Ext4_Data2: clean, 28/25600 files, 3268/25600 blocks
/dev/pwbd0p3: status 0, rss 3328, real 0.004991, user 0.000000, sys 0.004272
```

