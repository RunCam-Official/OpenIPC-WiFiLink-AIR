# Makefile

#make install BUILD_VERSION=ruby
BUILD_VERSION ?= wifilink
OPENIPC_DIR=/path/to/your/openipc/sdk/builder/openipc

ifeq ("$(wildcard $(OPENIPC_DIR))", "")
$(error OPENIPC_DIR path does not exist: "$(OPENIPC_DIR)")
endif

CC=arm-linux-gnueabihf-sigmastar-11.1.0-gcc
CFLAGS=-Wall -Iinc -I$(OPENIPC_DIR)/output/build/majestic-plugins-HEAD/sigmastar/include
LDFLAGS += -L$(OPENIPC_DIR)/general/package/sigmastar-osdrv-infinity6e/files/lib
LDLIBS += -lpthread -lmi_isp -lmi_sys -lmi_vpe -lcam_os_wrapper -lcus3a -lispalgo
SRCDIR=src
RESDIR=res
SOURCES=$(wildcard $(SRCDIR)/*.c)
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=wifilink

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(LDLIBS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(EXECUTABLE)

install:
	install -m 755 $(EXECUTABLE) $(OPENIPC_DIR)/general/overlay/usr/sbin/
ifeq ($(BUILD_VERSION), wifilink)
ifeq ($(wildcard $(OPENIPC_DIR)/general/overlay/etc/ruby-version), $(OPENIPC_DIR)/general/overlay/etc/ruby-version)
	rm $(OPENIPC_DIR)/general/overlay/etc/ruby-version
endif
	install -m 755 $(RESDIR)/user_$(BUILD_VERSION).ini $(OPENIPC_DIR)/general/overlay/etc/user.ini
endif
ifeq ($(BUILD_VERSION), ruby)
ifeq ($(wildcard $(OPENIPC_DIR)/general/overlay/etc/user.ini), $(OPENIPC_DIR)/general/overlay/etc/user.ini)
	rm $(OPENIPC_DIR)/general/overlay/etc/user.ini
endif
ifeq ($(wildcard $(OPENIPC_DIR)/output/target/etc/user.ini), $(OPENIPC_DIR)/output/target/etc/user.ini)
	rm $(OPENIPC_DIR)/output/target/etc/user.ini
endif
	install -m 755 $(RESDIR)/user_$(BUILD_VERSION).ini $(OPENIPC_DIR)/general/overlay/etc/ruby-version
endif
	install -m 755 $(RESDIR)/sysupgrade $(OPENIPC_DIR)/general/overlay/usr/sbin/
	install -m 755 $(RESDIR)/infinity6e-padmux-qfn.dtsi $(OPENIPC_DIR)/output/build/linux-custom/arch/arm/boot/dts/

	install -m 755 $(RESDIR)/S95majestic $(OPENIPC_DIR)/general/package/majestic/files/
	install -m 755 $(RESDIR)/S95majestic $(OPENIPC_DIR)/output/per-package/majestic/target/etc/init.d/
	install -m 755 $(RESDIR)/S95majestic $(OPENIPC_DIR)/output/target/etc/init.d/
ifeq ($(BUILD_VERSION), wifilink)
	install -m 755 $(RESDIR)/wfb_drone.conf $(OPENIPC_DIR)/general/package/wifibroadcast/files/
	install -m 755 $(RESDIR)/wfb_drone.conf $(OPENIPC_DIR)/output/per-package/wifibroadcast/target/etc/wfb.conf
	install -m 755 $(RESDIR)/wfb_drone.conf $(OPENIPC_DIR)/output/target/etc/wfb.conf

	install -m 755 $(RESDIR)/tweaksys $(OPENIPC_DIR)/general/package/datalink/files/
	install -m 755 $(RESDIR)/tweaksys $(OPENIPC_DIR)/output/per-package/datalink/target/usr/bin/
	install -m 755 $(RESDIR)/tweaksys $(OPENIPC_DIR)/output/target/usr/bin/

	install -m 755 $(RESDIR)/telemetry.conf $(OPENIPC_DIR)/output/per-package/datalink/target/etc/telemetry.conf
	install -m 755 $(RESDIR)/telemetry.conf $(OPENIPC_DIR)/output/target/etc/telemetry.conf
endif
	install -m 755 $(RESDIR)/IQ/SSC338Q+IMX415-a5296d01eb86a5417efa389a94dda0e4.bin $(OPENIPC_DIR)/general/package/sigmastar-osdrv-infinity6e/files/sensor/configs/imx415_fpv.bin
	install -m 755 $(RESDIR)/IQ/SSC338Q+IMX415-a5296d01eb86a5417efa389a94dda0e4.bin $(OPENIPC_DIR)/output/per-package/sigmastar-osdrv-infinity6e/target/etc/sensors/imx415_fpv.bin
	install -m 755 $(RESDIR)/IQ/SSC338Q+IMX415-a5296d01eb86a5417efa389a94dda0e4.bin $(OPENIPC_DIR)/output/target/etc/sensors/imx415_fpv.bin

.PHONY: all clean install
	