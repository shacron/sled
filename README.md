# sled
Sled System Emulator

Sled (SLow Emulated Device) is a library intended to make full system emulation of embedded products simple. It includes an emulated CPU and emulated devices. The intention of this project is to build a development and debug environment for low-level system software, where debugging capabilities are otherwise limited.

## Simple Machine

The default project builds an app named __sled__, which constructs a simple machine. The machine instantiates a core, loads an ELF binary of any supported architecture, and executes it. More complex machines can be defined programatically.

## Status

### RISCV

RISCV cores are an advanced state. They are able to run standalone binaries (i.e., things that don't require mode changes or generally access CSRs).

Supported:

* rv32i
* rv64i
* M, C extensions
* machine mode
* interrupts

In progress:

* system and user modes
* A extension
* exceptions
* most of the CSRs
* timers
* MMU/MPU

Long term:
* floating point support
* SMP
* hypervisor mode

### ARM

Not started.

### Devices

There are few generic devices implemented.

Supported:

* UART (serial)
* Interrupt Controller
* Real-time clock

In progress:

* Bus identification of master
* DMA backend and generic interface
* MMU backend

Long term goals:

* iovirt-* (particularly block)
* common embedded IP (designware, primecell, etc).
* IOMMU
* USB controller (device)

Users can add additional devices, though the API is not yet stable.


## Components

### Machine

The __machine__ component allows a user to create and configure a machine. A base machine includes only a __bus__ device. The cores, devices, memory, and the associated base addresses for these components are configurable by the user.

### Core

The __core__ component is an ISA-level software emulator of a CPU core. Currently RISCV (rv32 and rv64) is implemented, as well as a few extensions. A user defines the core configuration they desire and the machine will create it. They can then manipulate the core's register state, memory, and direct it to execute instructions.

### Bus and Memory

The __bus__ is the memory transaction dispatcher. It routes memory accesses to the intended memory or device. This emulates physical memory addressing. Users define memory configurations and add them to their machine.

### Devices

Sled includes a simple __device__ model. Devices implement MMIO functions to respond to memory transactions, as well as generating interrupts. Some devices are defined by sled (uart, interrupt controller, real time clock, etc) with simple interfaces. Others will be intended to emulate real hardware. Users can instantiate existing devices or implement their own and add them to a machine.

## Comparison to Existing Projects

Sled is not a VM and does not rely on any virtualization framework. It is purely a userspace application.

In some ways this project is similar to QEMU, but is intended to serve a different purpose. QEMU will be significantly faster in almost all cases. Sled is intended to make whole system introspection and debugging possible, not to perform at maximum speed.

That being said, sled is not that slow. At last test it could execute 35 million instructions per second on an emulated rv32 core when benchmarked on an Apple M1 processor. Scaling for multiple emulated cores should be fairly linear as long as there is a similar number of physical cores in the host machine.


## Building

### sledKit

Sled can be built standalone. However, for most people it would be easier to build it through the accompanying project, __sledKit__. sledKit is intended to help bootstrap a cross-compilation environment that will allow users to easily build and run binaries with sled.

https://github.com/shacron/sledkit

### Standalone

    make -j <jobs>

The following build options are for people developing sled:

    BUILD=debug

Build the project in debug mode for a better debugging experience.

    RV_TRACE=1

Enable RISCV instruction tracing. Running sled will print an instruction trace for every instruction dispatched. This significantly slows down execution.

## License

Sled is Copyright (c) 2022, Shac Ron.

Unless otherwise stated, all code is licensed under the MIT License. The short version that you can use this code for any purpose, but should retain the copyright notice in the source.

ELF header files (elf32.h and elf64.h) are Copyright (c) 1996-1998 John D. Polstra and licensed under BSD-2-Clause-FreeBSD. See the license included within these files for details.
