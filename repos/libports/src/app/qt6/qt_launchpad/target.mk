QMAKE_PROJECT_FILE = $(PRG_DIR)/qt_launchpad.pro

QMAKE_TARGET_BINARIES = qt_launchpad

QT6_PORT_LIBS += libQt6Core libQt6Gui libQt6Widgets

LIBS = base libc libm mesa stdcxx launchpad

include $(call select_from_repositories,lib/import/import-qt6_qmake.mk)

QT6_GENODE_LIBS_APP += ld.lib.so launchpad.lib.a
QT6_GENODE_LIBS_APP := $(filter-out qt6_component.lib.so,$(QT6_GENODE_LIBS_APP))

qmake_prepared.tag: qmake_root/lib/ld.lib.so qmake_root/lib/launchpad.lib.a
