# sled
Sled System Emulator

Sled (SLow Emulated Device) is a library intended to make full system emulation simple. It includes an emulated CPU and emulated devices. The intention of this project is to build a development and debug environment for low-level system software, where debugging capabilities are otherwise limited.

## Simple Machine

The default project builds an app named __sled__, which constructs a simple machine. The machine instantiates a core, loads an ELF binary of any supported architecture, and executes it. More complex machines can be defined programatically.

## Components

### Machine

The __machine__ component is the user API for sled, allowing a user to create and configure a machine. A base machine includes only a __bus__ device. The cores, devices, memory, and the associated base addresses for these components are configurable by the user.

### Core

The __core__ component is an ISA-level software emulator of a CPU core. Currently RISCV (rv32 and rv64) is implemented, as well as a few extensions. A user defines the core configuration they need and the machine will create it. They can then manipulate the core's register state, memory, and direct it to execute instructions.

### Bus and Memory

The __bus__ is the memory transaction dispatcher. It routes memory accesses to the intended memory or device. This emulates physical memory addressing. Users define memory configurations and add them to their machine.

### Devices

Sled includes a simple __device__ model. Devices implement MMIO functions to respond to memory transactions, as well as generating interrupts. Some devices are defined by sled (uart, interrupt controller, real time clock, etc) with simple interfaces. Others will be intended to emulate real hardware. Users can instantiate existing devices or implement their own and add them to a machine.

## Comparison to Existing Projects

Sled is not a VM and does not rely on any virtualization framework. It is purely a userspace application.

In some ways this project is similar to QEMU, but is intended to serve a different purpose. QEMU will be significantly faster in almost all cases. It is intended to make whole system introspection and debugging possible, not to perform at maximum speed.

That being said, sled is not that slow. At last test it could execute 35 million instructions per second on an emulated rv32 core when benchmarked on an Apple M1 processor. Scaling for multiple emulated cores should be fairly linear as long as there is a similar number of physical cores in the host machine.


## Building

### sledKit

Sled can be built standalone. However, for most people it would be easier to build it through the accompanying project, __sledKit__. sledKit is intended to help bootstrap a cross-compilation environment that will allow users to easily build and run binaries with sled.

SLEDKIT LINK HERE...

### Standalone

    make -j <num>

The following build options are for people developing sled:

    BUILD=debug

Build the project in debug mode for a better debugging experience.

    RV_TRACE=1

Enable RISCV instruction tracing. Running sled will print an instruction trace for every instruction dispatched. This significantly slows down execution.
