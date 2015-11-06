MRUBY_ROOT:=./mruby
SOFTWARE:=mruby-extension-module-for-miracle-zbx
VERSION:=$(shell grep "^Version:" mruby-extension-module-for-miracle-zbx.spec |grep -o "[0-9.]*")
TMP_BUILD_DIR:=.build
BUILD_DIR:=build

MRUBY_MAK_FILE := $(MRUBY_ROOT)/build/host/lib/libmruby.flags.mak
ifeq ($(wildcard $(MRUBY_MAK_FILE)),)
  MRUBY_CFLAGS =
  MRUBY_LDFLAGS =
  MRUBY_LIBS =
else
  include $(MRUBY_MAK_FILE)
endif

CFLAGS=$(INC) $(MRUBY_CFLAGS)
LDFLAGS=$(MRUBY_LDFLAGS) -lstdc++
LIBS=$(LIB) $(MRUBY_LIBS)
so:
	gcc $(CFLAGS) mruby_extension_module.c $(LDFLAGS) $(LIBS) $(APXS_LDFLAGS) -shared -fPIC -o mruby_extension_module.so
rpm: dist
	mkdir -p $(BUILD_DIR)/rpmbuild/BUILD $(BUILD_DIR)/rpmbuild/RPMS $(BUILD_DIR)/rpmbuild/SOURCES $(BUILD_DIR)/rpmbuild/SPECS $(BUILD_DIR)/rpmbuild/SRPMS
	cp $(SOFTWARE).spec $(BUILD_DIR)/rpmbuild/SPECS/
	cp $(BUILD_DIR)/$(SOFTWARE)-$(VERSION).tar.gz $(BUILD_DIR)/rpmbuild/SOURCES/
	rpmbuild -ba --define "_topdir `pwd`/$(BUILD_DIR)/rpmbuild/" $(BUILD_DIR)/rpmbuild/SPECS/$(SOFTWARE).spec
	cp -r $(BUILD_DIR)/rpmbuild/RPMS/* $(BUILD_DIR)/
	cp -r $(BUILD_DIR)/rpmbuild/SRPMS/ $(BUILD_DIR)/
	$(RM) -r $(BUILD_DIR)/rpmbuild/
dist: clean so
	mkdir -p $(TMP_BUILD_DIR)/$(SOFTWARE)-$(VERSION)/
	cp -Rp ./* $(TMP_BUILD_DIR)/$(SOFTWARE)-$(VERSION)/
	cd $(TMP_BUILD_DIR)/; tar czf $(SOFTWARE)-$(VERSION).tar.gz $(SOFTWARE)-$(VERSION)/ --exclude ".git"; $(RM) -r $(SOFTWARE)-$(VERSION)/
	mv $(TMP_BUILD_DIR)/ $(BUILD_DIR)/
clean:
	$(RM) -r $(TMP_BUILD_DIR)/
	$(RM) -r $(BUILD_DIR)/
