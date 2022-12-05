# sled

## Sled System Emulator

Sled (SLow Emulated Device) is a library intended to make full system emulation of embedded targets simple. It includes an emulated CPU and emulated devices. The intention of this project is to build a development and debug environment for low-level system software, where debugging capabilities are otherwise limited.

For fast building and use, see the __sledKit__ link below.

The project builds a library (__libsled.a__) which can be used to programatically construct a machine, specifying the CPUs, devices, memory offsets, RAM, and interrupt routing (TODO on that last one). The `simple machine` application provides an example of this. It should be straightforward (and probably implemented sometime in the future) to wrap this in a command-line argument parser or a device tree interpreter.

The goal of this machine is to simulate real-world hardware sufficiently to run native binaries on them, and to enable low level development and debugging of code for these targets. As such, __sled__ does not try to create the fastest emulated CPU. It instead aims to make a system where the state of all components can be examined and errors can be analyzed better than on real hardware. It is my hope that this product can be a useful tool for embedded engineers, myself included, and allow them to get their code done ahead of the hardware rather than behind it.

## Simple Machine

The default project builds an app named __sled__, which constructs a simple machine. The machine instantiates a core, loads an ELF binary of any supported architecture, and executes it. More complex machines can be defined programatically.

## Status

### RISCV

RISCV cores are at an advanced state. They are able to run standalone binaries (i.e., things that don't require mode changes or generally access CSRs).

Supported:

* rv32i
* rv64i
* M, C extensions
* machine mode
* interrupts
* (some) exceptions

In progress:

* more exceptions
* system and user modes
* A extension
* most of the CSRs
* timers
* MMU/MPU

Long term:
* floating point support
* SMP
* hypervisor mode

### ARM

Not started. Waiting for the core API to settle before starting this.

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

* iovirt-* (particularly console and block)
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

## Contributing

If you'd like to contribute code, there are many things to do. If it's a big thing, best to file an issue.

Good contributions opportunities:
* Tests! We need tests. There are some RISCV tests in __sledKit__ but coverage is lacking. Porting the rest of the Imperas RISCV conformance test suite is a good start.
* Virtio devices and drivers.
* Emulate some common devices (ex: DesignWare or PrimeCell IP)
* Other devices: SATA host controller, RNG, cryptographic accelerator, DAC, etc.
* Optimize memory address lookup in the __bus__ device.
* Memory transaction tracing and identification. Carry information about the issuer to the device.
* GDB stub - implement the GDB and LLDB protocol for symbolic debugging of code running on cores.
* JTAG interface - implement a port to allow JTAG debugging of the machine.
* A more complete application that takes machine descriptions from command line, device tree, or markup file.
* Port an OS to sled. Until there's an MMU or MPU it's not likely to go well for big OSs, but if you have something smaller, bring it.
* Build with different toolchains. Plugging in GCC should be pretty straightforward with few changes. Ideally this should be supported in __sledKit__ and just work here. Got other toolchains?

All code contributions (excluding properly licensed ports) must include a copyright assignment to The Sled Project. The author may include an additional copyright notice crediting themselves.


## License

Sled is Copyright (c) 2022, Shac Ron and The Sled Project.

Unless otherwise stated, all code is licensed under the MIT License. The short version is that you can use this code for any purpose, but should retain the copyright notice in the source.

ELF header files (elf32.h and elf64.h) are Copyright (c) 1996-1998 John D. Polstra and licensed under BSD-2-Clause-FreeBSD. See the license included within these files for details.
