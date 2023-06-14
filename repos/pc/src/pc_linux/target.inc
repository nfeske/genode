#
# Linux kernel configuration
#

# kernel fundamentals
LX_ENABLE += TTY SERIAL_EARLYCON SERIAL_OF_PLATFORM PRINTK HAS_IOMEM SMP

# support disabling ticking during idle
LX_ENABLE  += NO_HZ_IDLE

# initrd support
LX_ENABLE += BINFMT_ELF BINFMT_SCRIPT BLK_DEV_INITRD

# UART device
LX_ENABLE += SERIAL_8250 SERIAL_8250_CONSOLE

# PCI support
LX_ENABLE += PCI PCI_MSI

# IOMMU support (used by Intel fb to apply scanout scratch page workaround)
LX_ENABLE += IOMMU_SUPPORT INTEL_IOMMU

# ACPI support
LX_ENABLE += ACPI

# GPIO support
LX_ENABLE += GPIOLIB GPIO_ACPI

# general network and WLAN support
LX_ENABLE += NET NETDEVICES PACKET
LX_ENABLE += WLAN CFG80211 MAC80211
LX_ENABLE += RFKILL

# iwlwifi
LX_ENABLE += WLAN_VENDOR_INTEL IWLWIFI IWLDVM IWLMVM

# rtlwifi
LX_ENABLE += WLAN_VENDOR_REALTEK RTL_CARDS RTL8188EE

# ath9k
LX_ENABLE += WLAN_VENDOR_ATH ATH_COMMON ATH9K ATH9K_PCI
LX_ENABLE += ATH9K_DEBUGFS
#LX_ENABLE += ATH9K_RFKILL ATH9K_PCI_NO_EEPROM
#LX_ENABLE += ATH9K_HTC

# USB host-controller support (keep USB_USBNET hook)
LX_ENABLE += USB_USBNET
LX_ENABLE += USB_SUPPORT USB USB_ARCH_HAS_HCD USB_XHCI_HCD USB_EHCI_HCD USB_OHCI_HCD
LX_ENABLE += USB_PCI USB_UHCI_HCD

# Intel framebuffer
LX_ENABLE += DRM DRM_I915

# required for GPIO irqchip on tigerlake
LX_ENABLE += PINCTRL PINCTRL_TIGERLAKE

# I2C HID touchpad support (keep INPUT_EVDEV hook)
LX_ENABLE += INPUT_EVDEV HID_GENERIC
LX_ENABLE += MFD_INTEL_LPSS_PCI
LX_ENABLE += I2C_DESIGNWARE_PLATFORM
LX_ENABLE += I2C I2C_HID_ACPI

# no AML code is run that uses this feature
LX_DISABLE += ACPI_I2C_OPREGION

# to automatically set up screen mode at boot time
LX_ENABLE += FB

# mandatory fs support
LX_ENABLE += PROC_FS SYSFS

# Options for testing pc_linux images
#
# - EFI support for tests on EFI-only devices
#LX_ENABLE += EFI EFI_STUB
# - network support for NFS root
#LX_ENABLE += INET IP_PNP IP_PNP_DHCP NFS_FS FILE_LOCKING MULTIUSER ROOT_NFS
# - log all input events (requires "debug" on the kernel commandline)
#LX_ENABLE += INPUT_EVBUG
# - in-kernel symbolic backtraces with show_stack(NULL, NULL, KERN_DEFAULT)
#LX_ENABLE += KALLSYMS

# disable cpu frequency scaling
LX_DISABLE += SCHED_MC CPU_FREQ

# disable optimization not possible in PIC code under Genode
LX_DISABLE += CC_HAS_ASM_GOTO

# slim down kernel by removing superfluous drivers
LX_DISABLE += USB_HID VGA_CONSOLE DUMMY_CONSOLE NLS VGA_ARB DEBUG_LIST
